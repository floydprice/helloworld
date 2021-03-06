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

#ifndef __ZMQ_POLL_THREAD_HPP_INCLUDED__
#define __ZMQ_POLL_THREAD_HPP_INCLUDED__

#include <zmq/platform.hpp>

#if defined ZMQ_HAVE_LINUX || defined ZMQ_HAVE_FREEBSD ||\
    defined ZMQ_HAVE_OPENBSD || defined ZMQ_HAVE_SOLARIS ||\
    defined ZMQ_HAVE_OSX || defined ZMQ_HAVE_QNXNTO ||\
    defined ZMQ_HAVE_HPUX || defined ZMQ_HAVE_AIX

#include <poll.h>
#include <stddef.h>
#include <vector>

#include <zmq/poller.hpp>
#include <zmq/fd.hpp>

namespace zmq
{

    //  Implements socket polling mechanism using the POSIX.1-2001
    //  poll() system call. The class is used when instatntiating the poller
    //  template to generate the poll_thread_t class.

    class poll_t
    {
    public:

        poll_t ();
        virtual ~poll_t () {}

        handle_t add_fd (fd_t fd_, i_pollable *engine_);
        void rm_fd (handle_t handle_);
        void set_pollin (handle_t handle_);
        void reset_pollin (handle_t handle_);
        void set_pollout (handle_t handle_);
        void reset_pollout (handle_t handle_);
        bool process_events (poller_t <poll_t> *poller_, bool timers_);

    private:

        struct fd_entry_t
        {
            fd_t index;
            i_pollable *engine;
        };

        //  This table stores data for registered descriptors.
        std::vector <fd_entry_t> fd_table;

        //  Pollset to pass to the poll function.
        typedef std::vector <pollfd> pollset_t;
        pollset_t pollset;

        //  If true, there's at least one retired event source.
        bool retired;

        poll_t (const poll_t&);
        void operator = (const poll_t&);
    };

    typedef poller_t <poll_t> poll_thread_t;

}

#endif

#endif
