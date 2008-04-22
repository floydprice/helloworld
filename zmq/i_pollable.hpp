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

#ifndef __ZMQ_I_POLLABLE_HPP_INCLUDED__
#define __ZMQ_I_POLLABLE_HPP_INCLUDED__

#include "i_signaler.hpp"
#include "i_thread.hpp"
#include "command.hpp"

namespace zmq
{

    //  Virtual interface to be exposed by engines for communication with
    //  poll thread.
    struct i_pollable
    {
        //  The destructor shouldn't be virtual, however, not defining it as
        //  such results in compiler warnings with some compilers.
        virtual ~i_pollable () {};

        //  Used by owner thread to pass reference to itself to the engine
        virtual void set_thread (struct i_thread *thread_) = 0;

        //  Returns file descriptor to be used by poll thread to poll on
        virtual int get_fd () = 0;

        //  Returns events poll thread should poll for
        virtual short get_events () = 0;

        //  Called by poll thread when in event occurs
        virtual void in_event () = 0;

        //  Called by poll thread when out event occurs
        virtual void out_event () = 0;

        //  Called when command from a different thread is received
        //  TODO: Shouldn't we move this method to a separate inteface?
        //        Possibly being a base class for i_pollable?
        virtual void process_command (
            const struct engine_command_t &command_) = 0;
    };

}

#endif
