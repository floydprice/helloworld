/*
    Copyright (c) 2007-2008 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mux.hpp"
#include "raw_message.hpp"

zmq::mux_t::mux_t (int hal_, int lal_) :
    current (0),
    queue_size (0),
    hal (hal_),
    lal (lal_),
    alert_queued (false),
    alert_sent (false)
{
}

zmq::mux_t::~mux_t ()
{
}

void zmq::mux_t::receive_from (pipe_t *pipe_)
{
    //  Pass the mux pointer to the pipe.
    pipe_->set_mux (this);

    //  Associate new pipe with the mux object.
    pipes.push_back (pipe_);
}

bool zmq::mux_t::read (message_t *msg_)
{
    //  Underlying layers work with raw_message_t, layers above use messge_t.
    //  Mux is the component that translates between the two.
    raw_message_t *msg = (raw_message_t*) msg_;

    //  Deallocate old content of the message.
    raw_message_destroy (msg);

    //  If alert is to be sent, return it instead of regular message.
    if (alert_queued) {
printf ("Alert dispatched\n");
        raw_message_init_alert (msg);
        alert_queued = false;
        return true;
    }

    //  Round-robin over the pipes to get next message.
    for (int to_process = pipes.size (); to_process != 0; to_process --) {

        bool retrieved = pipes [current]->read ((raw_message_t*) msg_);

        if (pipes [current]->eop ()) {
            delete pipes [current];
            pipes.erase (pipes.begin () + current);
        } else
            current ++;
        if (current == pipes.size ())
            current = 0;

        if (retrieved) {
            if (hal) {
                queue_size --;
                if (queue_size <= lal)
                    alert_sent = false;
            }
            return true;
        }
    }

    //  No message is available. Initialise the output parameter
    //  to be a 0-byte message.
    raw_message_init (msg, 0);
    return false;
}

void zmq::mux_t::adjust_queue_size (int delta_)
{
printf ("Old queue size: %d\n", (int) queue_size);
printf ("Delta: %d\n", delta_);
    queue_size += delta_;
printf ("New queue size: %d\n", (int) queue_size);
printf ("Hal: %d\n", (int) hal);

    if (hal && queue_size > hal && !alert_sent) {
printf ("Alert queued\n");
        alert_queued = true;
        alert_sent = true;
    }
}

void zmq::mux_t::terminate_pipes()
{
    for (pipes_t::size_type i = 0; i < pipes.size () ; i ++)
        pipes [i]->send_destroy_pipe ();
}
