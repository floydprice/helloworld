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

#if defined ZMQ_HAVE_LINUX || defined ZMQ_HAVE_FREEBSD ||\
    defined ZMQ_HAVE_OPENBSD || defined ZMQ_HAVE_SOLARIS ||\
    defined ZMQ_HAVE_OSX || defined ZMQ_HAVE_QNXNTO

#include <sys/resource.h>
#include <poll.h>

#include <zmq/err.hpp>
#include <zmq/poll_thread.hpp>

zmq::poll_t::poll_t ()
{
    //  Get the limit on open file descriptors. Resize fds so that it
    //  can hold all descriptors.
    rlimit rl;
    int rc = getrlimit (RLIMIT_NOFILE, &rl);
    errno_assert (rc != -1);
    fd_table.resize (rl.rlim_cur);

    for (rlim_t i = 0; i < rl.rlim_cur; i ++)
        fd_table [i].index = -1;
}

zmq::cookie_t zmq::poll_t::add_fd (int fd_, void *udata_)
{
    pollfd pfd = {fd_, 0, 0};
    pollset.push_back (pfd);
    assert (fd_table [fd_].index == -1);

    fd_table [fd_].index = pollset.size() - 1;
    fd_table [fd_].udata = udata_;

    cookie_t cookie;
    cookie.fd = fd_;
    return cookie;
}

void zmq::poll_t::rm_fd (cookie_t cookie_)
{
    //  Remove the descriptor from pollset and fd table.
    int index = fd_table [cookie_.fd].index;
    assert (index != -1);
    pollset.erase (pollset.begin () + index);

    //  Mark the fd as unused.
    fd_table [cookie_.fd].index = -1;

    //  Adjust fd table to match new indices to the pollset. To make it more
    //  efficient we are traversing the pollset which is shorter than
    //  fd list itself.
    for (pollset_t::size_type i = index; i < pollset.size (); i ++)
        fd_table [pollset [i].fd].index = i;
}

void zmq::poll_t::set_pollin (cookie_t cookie_)
{
    int index = fd_table [cookie_.fd].index;
    pollset [index].events |= POLLIN;
}

void zmq::poll_t::reset_pollin (cookie_t cookie_)
{
    int index = fd_table [cookie_.fd].index;
    pollset [index].events &= ~((short) POLLIN);
}

void zmq::poll_t::set_pollout (cookie_t cookie_)
{
    int index = fd_table [cookie_.fd].index;
    pollset [index].events |= POLLOUT;
}

void zmq::poll_t::reset_pollout (cookie_t cookie_)
{
    int index = fd_table [cookie_.fd].index;
    pollset [index].events &= ~((short) POLLOUT);
}

void zmq::poll_t::wait (event_list_t &event_list_)
{
    //  Wait for events.
    int rc = poll (&pollset [0], pollset.size (), -1);
    errno_assert (rc != -1);

    for (pollset_t::size_type i = 0; i < pollset.size (); i ++) {
        int fd = pollset [i].fd;

        assert (!(pollset [i].revents & POLLNVAL));

        if (pollset [i].revents & (POLLERR | POLLHUP)) {
            event_t ev = {fd, ZMQ_EVENT_ERR, fd_table [fd].udata};
            event_list_.push_back (ev);
        }
        if (pollset [i].revents & POLLOUT) {
            event_t ev = {fd, ZMQ_EVENT_OUT, fd_table [fd].udata};
            event_list_.push_back (ev);
        }
        if (pollset [i].revents & POLLIN) {
            event_t ev = {fd, ZMQ_EVENT_IN, fd_table [fd].udata};
            event_list_.push_back (ev);
        }
    }
}

#endif
