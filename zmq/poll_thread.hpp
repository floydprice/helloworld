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

#ifndef __ZMQ_POLL_THREAD_HPP_INCLUDED__
#define __ZMQ_POLL_THREAD_HPP_INCLUDED__

#include <stddef.h>
#include <assert.h>
#include <pthread.h>
#include <poll.h>

#include "dispatcher.hpp"
#include "dispatcher_proxy.hpp"
#include "ysocketpair.hpp"
#include "i_pollable.hpp"

namespace zmq
{

    class poll_thread_t 
    {
    public:

        poll_thread_t (i_pollable *engine_);
        ~poll_thread_t ();

    protected:

        enum {stop_event = 0xff};

        static void *worker_routine (void *arg_);
        void loop ();

        i_pollable *engine;
        ysocketpair_t signaler;
        pthread_t worker;
    };

}

#endif
