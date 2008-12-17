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

#include <string.h>

#ifdef ZMQ_HAVE_WINDOWS
#include "winsock2.h"
#elif defined ZMQ_HAVE_HPUX
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#else
#include <sys/select.h>
#endif

#include <zmq/err.hpp>
#include <zmq/select_thread.hpp>
#include <zmq/fd.hpp>

zmq::select_t::select_t () :
    maxfd (RETIRED_FD),
    retired (false)
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
    assert (it != fds.end ());
    it->fd = RETIRED_FD;
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

bool zmq::select_t::process_events (poller_t <select_t> *poller_)
{
    memcpy (&readfds, &source_set_in, sizeof source_set_in);
    memcpy (&writefds, &source_set_out, sizeof source_set_out);
    memcpy (&exceptfds, &source_set_err, sizeof source_set_err);

    while (true) {
        int rc = select (maxfd + 1, &readfds, &writefds, &exceptfds, NULL);
#ifdef ZMQ_HAVE_WINDOWS
        wsa_assert (rc != SOCKET_ERROR);
#else
        errno_assert (rc != -1);
#endif
        if (rc > 0)
            break;
    }

    for (fd_set_t::size_type i = 0; i < fds.size (); i ++) {
        if (fds [i].fd == RETIRED_FD)
            continue;
        if (FD_ISSET (fds [i].fd, &writefds))
            if (poller_->process_event (fds [i].engine, event_out))
                return true;
        if (fds [i].fd == RETIRED_FD)
            continue;
        if (FD_ISSET (fds [i].fd, &readfds))
            if (poller_->process_event (fds [i].engine, event_in))
                return true;
        if (fds [i].fd == RETIRED_FD)
            continue;
        if (FD_ISSET (fds [i].fd, &exceptfds))
            if (poller_->process_event (fds [i].engine, event_err))
                return true;
    }

    //  Destroy retired event sources.
    if (retired) {
        for (fd_set_t::size_type i = 0; i < fds.size (); i ++) {
            if (fds [i].fd == RETIRED_FD) {
                fds.erase (fds.begin () + i);
                i --;
            }
        }
        retired = false;
    }

    return false;
}
