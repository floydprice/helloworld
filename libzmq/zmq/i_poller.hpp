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

#ifndef __ZMQ_I_POLLER_HPP_INCLUDED__
#define __ZMQ_I_POLLER_HPP_INCLUDED__

#include <zmq/fd.hpp>

namespace zmq
{

    union handle_t
    {
        fd_t fd;
        void *ptr;
    };

    //  Virtual interface to be used by file-descriptor-oriented engines
    //  for communication with I/O threads.

    struct i_poller
    {
        virtual ~i_poller () {};

        //  Add file descriptor to the polling set. Return handle
        //  representing the descriptor.
        virtual handle_t add_fd (fd_t fd_, struct i_pollable *engine_) = 0;

        //  Remove file descriptor identified by handle from the polling set.
        virtual void rm_fd (handle_t handle_) = 0;

        //  Start polling for input from socket.
        virtual void set_pollin (handle_t handle_) = 0;

        //  Stop polling for input from socket.
        virtual void reset_pollin (handle_t handle_) = 0;

        //  Start polling for availability of the socket for writing.
        virtual void set_pollout (handle_t handle_) = 0;

        //  Stop polling for availability of the socket for writing.
        virtual void reset_pollout (handle_t handle_) = 0;

        //  Ask to be notified after some time. Actual interval varies between
        //  0 and max_timer_period ms. Timer is destroyed once it expires or,
        //  optionally, when cancel_timer is called.
        virtual void add_timer (struct i_pollable *engine_) = 0;

        //  Cancel the timer set by add_timer method.
        virtual void cancel_timer (struct i_pollable *engine_) = 0;

        //  Following methods are to be used by I/O thread owenr. Please
        //  don't invoke them from individual engines.

        //  Start the execution of the underlying I/O thread.
        virtual void start () = 0;

        //  First function start asynchronous shutdown of the poller, second
        //  one waits till the shutdown is finished. If particular API thread
        //  allows only for synchronous shutdown, initialise_shutdown should
        //  be left empty and terminate_shutdown should contain actual code.
        virtual void initialise_shutdown () = 0;
        virtual void terminate_shutdown () = 0;
    };

}

#endif
