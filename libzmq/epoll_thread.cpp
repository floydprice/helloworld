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

#ifdef ZMQ_HAVE_LINUX

#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>

#include <zmq/err.hpp>
#include <zmq/config.hpp>
#include <zmq/epoll_thread.hpp>
#include <zmq/fd.hpp>

zmq::epoll_t::epoll_t ()
{
    epoll_fd = epoll_create (1);
    errno_assert (epoll_fd != -1);
}

zmq::epoll_t::~epoll_t ()
{
    close (epoll_fd);
}

zmq::handle_t zmq::epoll_t::add_fd (fd_t fd_, i_pollable *engine_)
{
    poll_entry_t *pe = new poll_entry_t;
    zmq_assert (pe != NULL);

    pe->fd = fd_;
    pe->ev.events = 0;
    pe->ev.data.ptr = pe;
    pe->engine = engine_;

    int rc = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_, &pe->ev);
    errno_assert (rc != -1);

    handle_t handle;
    handle.ptr = pe;
    return handle;
}

void zmq::epoll_t::rm_fd (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    int rc = epoll_ctl (epoll_fd, EPOLL_CTL_DEL, pe->fd, &pe->ev);
    errno_assert (rc != -1);
    pe->fd = retired_fd;
    retired.push_back (pe);
}

void zmq::epoll_t::set_pollin (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->ev.events |= EPOLLIN;
    int rc = epoll_ctl (epoll_fd, EPOLL_CTL_MOD, pe->fd, &pe->ev);
    errno_assert (rc != -1);
}

void zmq::epoll_t::reset_pollin (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->ev.events &= ~((short) EPOLLIN);
    int rc = epoll_ctl (epoll_fd, EPOLL_CTL_MOD, pe->fd, &pe->ev);
    errno_assert (rc != -1);
}

void zmq::epoll_t::set_pollout (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->ev.events |= EPOLLOUT;
    int rc = epoll_ctl (epoll_fd, EPOLL_CTL_MOD, pe->fd, &pe->ev);
    errno_assert (rc != -1);
}

void zmq::epoll_t::reset_pollout (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_.ptr;
    pe->ev.events &= ~((short) EPOLLOUT);
    int rc = epoll_ctl (epoll_fd, EPOLL_CTL_MOD, pe->fd, &pe->ev);
    errno_assert (rc != -1);
}

void zmq::epoll_t::add_timer (i_pollable *engine_)
{
     timers.push_back (engine_);
}

void zmq::epoll_t::cancel_timer (i_pollable *engine_)
{
    timers_t::iterator it = std::find (timers.begin (), timers.end (), engine_);
    if (it == timers.end ())
        return;
    timers.erase (it);
}

bool zmq::epoll_t::process_events (poller_t <epoll_t> *poller_)
{
    epoll_event ev_buf [max_io_events];

    //  Wait for events.
    int n;
    while (true) {
        n = epoll_wait (epoll_fd, &ev_buf [0], max_io_events,
            timers.empty () ? -1 : max_timer_period);
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

        return false;
    }

    for (int i = 0; i < n; i ++) {
        poll_entry_t *pe = ((poll_entry_t*) ev_buf [i].data.ptr);

        if (pe->fd == retired_fd)
            continue;
        if (ev_buf [i].events & (EPOLLERR | EPOLLHUP))
            pe->engine->in_event ();
        if (pe->fd == retired_fd)
            continue;
        if (ev_buf [i].events & EPOLLOUT)
            pe->engine->out_event ();
        if (pe->fd == retired_fd)
            continue;
        if (ev_buf [i].events & EPOLLIN)
            if (pe->engine)
                pe->engine->in_event ();
            else
                if (!poller_->process_commands ())
                    return true;
    }

    //  Destroy retired event sources.
    for (retired_t::iterator it = retired.begin (); it != retired.end ();
          it ++)
        delete *it;
    retired.clear ();

    return false;
}

#endif
