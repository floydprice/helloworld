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

#if defined (ZMQ_HAVE_FREEBSD) || defined (ZMQ_HAVE_OPENBSD) ||\
    defined (ZMQ_HAVE_OSX)

#include <sys/time.h>
#include <sys/types.h>
#include <sys/event.h>
#include <stdlib.h>
#include <unistd.h>

#include <zmq/err.hpp>
#include <zmq/config.hpp>
#include <zmq/kqueue.hpp>
#include <zmq/fd.hpp>

zmq::kqueue_t::kqueue_t () :
    stopping (false);
{
    //  Create event queue
    kqueue_fd = kqueue ();
    errno_assert (kqueue_fd != -1);
}

zmq::kqueue_t::~kqueue_t ()
{
    close (kqueue_fd);
}

void zmq::kqueue_t::kevent_add (fd_t fd_, short filter_, void *udata_)
{
    struct kevent ev;

    EV_SET (&ev, fd_, filter_, EV_ADD, 0, 0, udata_);
    int rc = kevent (kqueue_fd, &ev, 1, NULL, 0, NULL);
    errno_assert (rc != -1);
}

void zmq::kqueue_t::kevent_delete (fd_t fd_, short filter_)
{
    struct kevent ev;

    EV_SET (&ev, fd_, filter_, EV_DELETE, 0, 0, NULL);
    int rc = kevent (kqueue_fd, &ev, 1, NULL, 0, NULL);
    errno_assert (rc != -1);
}

zmq::handle_t zmq::kqueue_t::add_fd (fd_t fd_, i_pollable *engine_)
{
    poll_entry_t *pe = new poll_entry_t;
    zmq_assert (pe != NULL);
    pe->fd = fd_;
    pe->flag_pollin = 0;
    pe->flag_pollout = 0;
    pe->engine = engine_;

    handle_t handle;
    handle.ptr = pe;
    return handle;
}

void zmq::kqueue_t::rm_fd (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    if (pe->flag_pollin)
        kevent_delete (pe->fd, EVFILT_READ);
    if (pe->flag_pollout)
        kevent_delete (pe->fd, EVFILT_WRITE);
    pe->fd = retired_fd;
    retired.push_back (pe);
}

void zmq::kqueue_t::set_pollin (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->flag_pollin = true;
    kevent_add (pe->fd, EVFILT_READ, pe);
}

void zmq::kqueue_t::reset_pollin (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->flag_pollin = false;
    kevent_delete (pe->fd, EVFILT_READ);
}

void zmq::kqueue_t::set_pollout (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->flag_pollout = true;
    kevent_add (pe->fd, EVFILT_WRITE, pe);
}

void zmq::kqueue_t::reset_pollout (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->flag_pollout = false;
    kevent_delete (pe->fd, EVFILT_WRITE);
}

void zmq::kqueue_t::add_timer (i_pollable *engine_)
{
     timers.push_back (engine_);
}

void zmq::kqueue_t::cancel_timer (i_pollable *engine_)
{
    timers_t::iterator it = std::find (timers.begin (), timers.end (), engine_);
    if (it == timers.end ())
        return;
    timers.erase (it);
}

void zmq::kqueue_t::start ()
{
    worker.start (worker_routine, this);
}

void zmq::kqueue_t::initialise_shutdown ()
{
    stopping = true;
}

void zmq::kqueue_t::terminate_shutdown ()
{
    worker.stop ();
}

void zmq::kqueue_t::loop ()
{
    struct kevent ev_buf [max_io_events];

    while (!stopping) {

        //  Compute time interval to wait.
        timespec timeout = {max_timer_period / 1000, 
            (max_timer_period % 1000) * 1000000};

        //  Wait for events.
        int n;
        while (true) {
            n = kevent (kqueue_fd, NULL, 0,
                &ev_buf [0], max_io_events, timers.empty () ? NULL : &timeout);
            if (!(n == -1 && errno == EINTR)) {
                errno_assert (n != -1);
                break;
            }
        }

        //  Handle timer.
        if (!n) {

            //  Use local list of timers as timer handlers may fill new timers
            //  into the original array.
            timers_t t;
            std::swap (timers, t);

            //  Trigger all the timers.
            for (timers_t::iterator it = t.begin (); it != t.end (); it ++)
                (*it)->timer_event ();

            return;
        }

        for (int i = 0; i < n; i ++) {
            poll_entry_t *pe = (poll_entry_t*) ev_buf [i].udata;

            if (pe->fd == retired_fd)
                continue;
            if (ev_buf [i].flags & EV_EOF)
                pe->engine->in_event ();
            if (pe->fd == retired_fd)
                continue;
            if (ev_buf [i].filter == EVFILT_WRITE)
                pe->engine->out_event ();
            if (pe->fd == retired_fd)
                continue;
            if (ev_buf [i].filter == EVFILT_READ)
                pe->engine->in_event ();
        }

        //  Destroy retired event sources.
        for (retired_t::iterator it = retired.begin (); it != retired.end ();
              it ++)
            delete *it;
        retired.clear ();
    }
}

void zmq::kqueue_t::worker_routine (void *arg_)
{
    ((kqueue_t*) arg_)->loop ();
}

#endif
