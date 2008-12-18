/*
    Copyright (c) 2007-2008 FastMQ Inc.

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
    poll_entry_t *pe = (poll_entry_t*) malloc (sizeof (poll_entry_t));
    assert (pe != NULL);

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

bool zmq::epoll_t::process_events (poller_t <epoll_t> *poller_)
{
    epoll_event ev_buf [max_io_events];

    //  Wait for events.
    int n = epoll_wait (epoll_fd, &ev_buf [0], max_io_events, -1);
    errno_assert (n != -1);

    for (int i = 0; i < n; i ++) {
        poll_entry_t *pe = ((poll_entry_t*) ev_buf [i].data.ptr);

        if (pe->fd == retired_fd)
            continue;
        if (ev_buf [i].events & (EPOLLERR | EPOLLHUP))
            if (poller_->process_event (pe->engine, event_err))
                return true;
        if (pe->fd == retired_fd)
            continue;
        if (ev_buf [i].events & EPOLLOUT)
            if (poller_->process_event (pe->engine, event_out))
                return true;
        if (pe->fd == retired_fd)
            continue;
        if (ev_buf [i].events & EPOLLIN)
            if (poller_->process_event (pe->engine, event_in))
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
