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

#include <limits>
#include "pipe.hpp"
#include "command.hpp"

zmq::pipe_t::pipe_t (i_context *source_context_, i_engine *source_engine_,
      i_context *destination_context_, i_engine *destination_engine_,
      int hwm_, int lwm_) :
    pipe (false),
    source_context (source_context_),
    source_engine (source_engine_),
    destination_context (destination_context_),
    destination_engine (destination_engine_),
    alive (true), 
    endofpipe (false),
    hwm (hwm_),
    lwm (lwm_),
    head (0),
    tail (0),
    last_head (0)
{
    //  If high water mark was lower than low water mark pipe would hang up.
    assert (lwm <= hwm);
}

zmq::pipe_t::~pipe_t ()
{
    //  Destroy the messages in the pipe itself
    raw_message_t message;
    pipe.flush ();
    while (pipe.read (&message))
        raw_message_destroy (&message);
}

bool zmq::pipe_t::check_write ()
{
    if (hwm) {

        //  If pipe size have reached high watermark, reject the write.
        //  The else branch will come into effect after 500,000 years of
        //  passing 1,000,000 messages a second but still, it's there...
        int size = last_head <= tail ? tail - last_head :
            std::numeric_limits <uint64_t>::max () - last_head + tail + 1;
        assert (size <= hwm);
        if (size == hwm)
            return false;
    }
    return true;
}

void zmq::pipe_t::write (raw_message_t *msg_)
{
    //  Physically write the message to the pipe.
    pipe.write (*msg_);

    //  When pipe limits are set, move the tail.
    if (hwm)
        tail ++;
}

void zmq::pipe_t::write_delimiter ()
{
    raw_message_t delimiter;
    raw_message_init_delimiter (&delimiter);
    pipe.write (delimiter);
    flush ();
}

void zmq::pipe_t::flush ()
{
    if (!pipe.flush ()) {
        command_t cmd;
        cmd.init_engine_revive (destination_engine, this);
        source_context->send_command (destination_context, cmd);
    }
}

void zmq::pipe_t::revive ()
{
    assert (!alive);
    alive = true;
}

void zmq::pipe_t::set_head (uint64_t position_)
{
    //  This may cause the next write to succeed.
    last_head = position_;
}

bool zmq::pipe_t::read (raw_message_t *msg_)
{
    //  If the pipe is dead, there's nothing we can do
    if (!alive)
        return false;

    //  Get next message, if it's not there, die.
    if (!pipe.read (msg_))
    {
        alive = false;
        return false;
    }

    //  If delimiter is read from the pipe, mark the pipe as ended
    if (msg_->content == (void*) raw_message_t::delimiter_tag) {
        endofpipe = true;
        return false;
    }

    //  Once in N messages send current head position to the writer thread.
    if (hwm) {
        head ++;

        //  If high water mark is same as low water mark we have to report each
        //  message retrieval from the pipe. Otherwise the period N is computed
        //  as a difference between high and low water marks.
        if (head % (hwm - lwm + 1) == 0) {
            command_t cmd;
            cmd.init_engine_head (source_engine, this, head);
            destination_context->send_command (source_context, cmd);
        }
    }

    return true;
}

void zmq::pipe_t::send_destroy_pipe ()
{
    command_t cmd;
    cmd.init_engine_destroy_pipe (source_engine, this);
    destination_context->send_command (source_context, cmd);
}

