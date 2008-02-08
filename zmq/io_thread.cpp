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

#include "io_thread.hpp"
#include "err.hpp"

zmq::io_thread_t::io_thread_t (dispatcher_t *dispatcher_, int thread_id_,
      i_engine *engine_) :
    engine (engine_),
    proxy (dispatcher_, thread_id_, &signaler)
{
    engine->set_dispatcher_proxy (&proxy);
    int rc = pthread_create (&worker, NULL, worker_routine, this);
    errno_assert (rc == 0);
}

zmq::io_thread_t::~io_thread_t ()
{
    signaler.signal (stop_event);

    int rc = pthread_join (worker, NULL);
    errno_assert (rc == 0);
}

void *zmq::io_thread_t::worker_routine (void *arg_)
{
    io_thread_t *self = (io_thread_t*) arg_;
    self->loop ();
    return 0;
}

void zmq::io_thread_t::loop ()
{
    bool stop = false;

    pollfd pfd [2];
    pfd [0].fd = signaler.get_fd ();
    pfd [0].events = POLLIN;
    pfd [1].fd = engine->get_fd ();
    pfd [1].events = (engine->get_in () ? POLLIN : 0) |
        (engine->get_out () ? POLLOUT : 0);

    while (true)
    {
        int rc = poll (pfd, 2, -1);
        errno_assert (rc != -1);
        assert (!(pfd [0].revents & (POLLERR | POLLHUP | POLLNVAL)));
        assert (!(pfd [1].revents & (POLLERR | POLLHUP | POLLNVAL)));

        if (pfd [0].revents & POLLIN) {

            unsigned char events [256];
            ssize_t nbytes = recv (pfd [0].fd, events, 256,
                MSG_DONTWAIT);
            errno_assert (nbytes != -1);
            for (int event = 0; event != nbytes; event ++) {
                if (events [event] == stop_event) {
                    if (!(pfd [1].events & POLLOUT)) //  if there are no messages to send, quit immediately
                        return;
                    stop = true;
                }
                else {
                    proxy.revive (events [event]);
                    if (engine->get_out())
                        pfd [1].events |= POLLOUT;
                }
            }
        }

        if (pfd [1].revents & POLLOUT)
            if (!engine->out_event ()) {
                pfd [1].events ^= POLLOUT;
                if (stop)
                    return;
            }

        if (pfd [1].revents & POLLIN)
            if (!engine->in_event ())
                pfd [1].events ^= POLLIN;
    }
}
