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


#ifndef __ZMQ_POLLABLE_FACTORY_HPP_INCLUDED__
#define __ZMQ_POLLABLE_FACTORY_HPP_INCLUDED__

#include <zmq/i_thread.hpp>
#include <zmq/i_pollable.hpp>
#include <zmq/i_listener.hpp>
#include <zmq/export.hpp>

namespace zmq
{

    ZMQ_EXPORT i_listener *create_listener (i_thread *calling_thread_,
        i_thread *thread_, const char *arguments_,
        int handler_thread_count_, i_thread **handler_threads_,
        bool source_, i_thread *peer_thread_, i_engine *peer_engine_,
        const char *peer_name_);

    ZMQ_EXPORT i_pollable *create_connection (i_thread *calling_thread_,
        i_thread *thread_, const char *arguments_, const char *local_object_,
        const char *engine_arguments_);

}

#endif
