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

#ifndef __ZMQ_KQUEUE_THREAD_HPP_INCLUDED__
#define __ZMQ_KQUEUE_THREAD_HPP_INCLUDED__

#include <zmq/platform.hpp>

#if defined (ZMQ_HAVE_FREEBSD) || defined (ZMQ_HAVE_OPENBSD) ||\
    defined (ZMQ_HAVE_OSX)

#include <zmq/poller.hpp>

namespace zmq
{

    //  Implements socket polling mechanism using the BSD-specific
    //  kqueue interface. This class is used to instantiate the poller
    //  template to generate the kqueue_thread_t class.

    class kqueue_t
    {
    public:

        kqueue_t ();
        ~kqueue_t ();

        handle_t add_fd (int fd_, i_pollable *engine_);
        void rm_fd (handle_t handle_);
        void set_pollin (handle_t handle_);
        void reset_pollin (handle_t handle_);
        void set_pollout (handle_t handle_);
        void reset_pollout (handle_t handle_);
        bool process_events (poller_t <kqueue_t> *poller_);

    private:

        //  Adds the event to the kqueue.
        void kevent_add (int fd_, short filter_, void *udata_);

        //  Deletes the event from the kqueue.
        void kevent_delete (int fd_, short filter_);

        //  File descriptor referring to the kernel event queue.
        int kqueue_fd;

        struct poll_entry_t
        {
            int fd;
            bool flag_pollin;
            bool flag_pollout;
            i_pollable *engine;
        };

        //  List of retired event sources.
        typedef std::vector <poll_entry_t*> retired_t;
        retired_t retired;

        kqueue_t (const kqueue_t&);
        void operator = (const kqueue_t&);
    };

    typedef poller_t <kqueue_t> kqueue_thread_t;

}

#endif

#endif
