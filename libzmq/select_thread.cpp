/*
    Copyright (c) 2007-2009 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <zmq/platform.hpp>

#include <algorithm>
#include <string.h>

#ifdef ZMQ_HAVE_WINDOWS
#include "winsock2.h"
#elif defined ZMQ_HAVE_HPUX
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#elif defined ZMQ_HAVE_OPENVMS
#include <sys/types.h>
#include <sys/time.h>
#else
#include <sys/select.h>
#endif

#include <zmq/err.hpp>
#include <zmq/select_thread.hpp>
#include <zmq/fd.hpp>

zmq::select_t::select_t () :
    maxfd (retired_fd),
    retired (false),
    stopping (false)
{
    //  Clear file descriptor sets.
    FD_ZERO (&source_set_in);
    FD_ZERO (&source_set_out);
    FD_ZERO (&source_set_err);
}

zmq::handle_t zmq::select_t::add_fd (fd_t fd_, i_pollable *engine_)
{
    //  Store the file descriptor.
    fd_entry_t entry = {fd_, engine_};
    fds.push_back (entry);

    //  Start polling on errors.
    FD_SET (fd_, &source_set_err);

    //  Adjust maxfd if necessary.
    if (fd_ > maxfd)
        maxfd = fd_;

    handle_t handle;
    handle.fd = fd_;
    return handle;
}

void zmq::select_t::rm_fd (handle_t handle_)
{
    //  Get file descriptor.
    fd_t fd = handle_.fd;

    //  Stop polling on the descriptor.
    FD_CLR (fd, &source_set_in);
    FD_CLR (fd, &source_set_out);
    FD_CLR (fd, &source_set_err);

    //  Discard all events generated on this file descriptor.
    FD_CLR (fd, &readfds);
    FD_CLR (fd, &writefds);
    FD_CLR (fd, &exceptfds);

    //  Adjust the maxfd attribute if we have removed the
    //  highest-numbered file descriptor.
    if (fd == maxfd) {
        maxfd = 0;
        for (fd_set_t::iterator it = fds.begin (); it != fds.end (); it ++)
            if (it->fd > maxfd)
                maxfd = it->fd;
    }

    //  Mark the descriptor as retired.
    fd_set_t::iterator it;
    for (it = fds.begin (); it != fds.end (); it ++)
        if (it->fd == fd)
            break;
    zmq_assert (it != fds.end ());
    it->fd = retired_fd;
    retired = true;
}

void zmq::select_t::set_pollin (handle_t handle_)
{
    FD_SET (handle_.fd, &source_set_in);
}

void zmq::select_t::reset_pollin (handle_t handle_)
{
    FD_CLR (handle_.fd, &source_set_in);
}

void zmq::select_t::set_pollout (handle_t handle_)
{
    FD_SET (handle_.fd, &source_set_out);
}

void zmq::select_t::reset_pollout (handle_t handle_)
{
    FD_CLR (handle_.fd, &source_set_out);
}

void zmq::select_t::add_timer (i_pollable *engine_)
{
     timers.push_back (engine_);
}

void zmq::select_t::cancel_timer (i_pollable *engine_)
{
    timers_t::iterator it = std::find (timers.begin (), timers.end (), engine_);
    if (it == timers.end ())
        return;
    timers.erase (it);
}

void zmq::select_t::start ()
{
    worker.start (worker_routine, this);
}

void zmq::select_t::initialise_shutdown ()
{
    stopping = true;
}

void zmq::select_t::terminate_shutdown ()
{
    worker.stop ();
}

void zmq::select_t::loop ()
{
    while (!stopping) {

        //  Intialise the pollsets.
        memcpy (&readfds, &source_set_in, sizeof source_set_in);
        memcpy (&writefds, &source_set_out, sizeof source_set_out);
        memcpy (&exceptfds, &source_set_err, sizeof source_set_err);

        //  Wait for events.
        while (true) {

            //  Compute the timout interval. Select is free to overwrite the
            //  value so have to compute it each time anew.
            timeval timeout = {max_timer_period / 1000,
                (max_timer_period % 1000) * 1000};

            //  Wait for events.
            int rc;
            while (true) {
                rc = select (maxfd + 1, &readfds, &writefds, &exceptfds,
                    timers.empty () ? NULL : &timeout);

#ifdef ZMQ_HAVE_WINDOWS
                wsa_assert (rc != SOCKET_ERROR);
                break;
#else
                if (!(rc == -1 && errno == EINTR)) {
                    errno_assert (rc != -1);
                    break;
                }
#endif
            }

            //  Handle timer.
            if (!timers.empty () && !rc) {

                //  Use local list of timers as timer handlers may fill
                //  new timers into the original array.
                timers_t t;
                std::swap (timers, t);

                //  Trigger all the timers.
                for (timers_t::iterator it = t.begin (); it != t.end (); it++)
                    (*it)->timer_event ();

                return;
            }

            //  TODO: Select sometimes returns 0 even though no event have
            //  occured and no timeout was set. Document this situation
            //  in detail...
            if (rc > 0)
                break;
        }

        for (fd_set_t::size_type i = 0; i < fds.size (); i++) {
            if (fds [i].fd == retired_fd)
                continue;
            if (FD_ISSET (fds [i].fd, &writefds))
                fds [i].engine->out_event ();
            if (fds [i].fd == retired_fd)
                continue;
            if (FD_ISSET (fds [i].fd, &readfds))
                fds [i].engine->in_event ();
            if (fds [i].fd == retired_fd)
                continue;
            if (FD_ISSET (fds [i].fd, &exceptfds))
                fds [i].engine->in_event ();
        }

        //  Destroy retired event sources.
        if (retired) {
            for (fd_set_t::size_type i = 0; i < fds.size (); i++) {
                if (fds [i].fd == retired_fd) {
                    fds.erase (fds.begin () + i);
                    i--;
                }
            }
            retired = false;
        }
    }
}

void zmq::select_t::worker_routine (void *arg_)
{
    ((select_t*) arg_)->loop ();
}

