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

#include <assert.h>
#include <string.h>

#include <zmq/err.hpp>
#include <zmq/locator.hpp>
#include <zmq/engine_factory.hpp>
#include <zmq/config.hpp>
#include <zmq/formatting.hpp>

zmq::locator_t::locator_t ()
{
}

zmq::locator_t::~locator_t ()
{
}

void zmq::locator_t::create (i_thread *calling_thread_,
    unsigned char type_id_, const char *object_,
    i_thread *thread_, i_engine *engine_, scope_t scope_,
    const char *interface_, i_thread *listener_thread_,
    int handler_thread_count_, i_thread **handler_threads_)
{
    assert (type_id_ < type_id_count);
    assert (strlen (object_) < 256);

    //  Enter critical section.
    sync.lock ();

    //  Add the object to the list of known objects.
    object_info_t info = {thread_, engine_};
    objects [type_id_].insert (objects_t::value_type (object_, info));

    //  Add the object to the global locator.
    if (scope_ == scope_global) {

        assert (strlen (interface_) < 256);
         
        //  Create a listener for the object.
        engine_factory_t::create_listener (
            calling_thread_, listener_thread_, interface_,
            handler_thread_count_, handler_threads_,
            type_id_ == exchange_type_id ? false : true,
            thread_, engine_, object_);
    }

    //  Leave critical section.
    sync.unlock ();
}

bool zmq::locator_t::get (i_thread *calling_thread_, unsigned char type_id_,
    const char *object_, const char *iface_, i_thread **thread_,
    i_engine **engine_, i_thread *handler_thread_, const char *local_object_,
    const char *engine_arguments_)
{
    assert (type_id_ < type_id_count);
    assert (strlen (object_) < 256);

    //  Enter critical section.
    sync.lock ();

    //  Find the object.
    objects_t::iterator it = objects [type_id_].find (object_);

    //  If the object is unknown, find it using global locator.
    if (it == objects [type_id_].end ()) {

        //  Create the proxy engine for the object.
        i_engine *engine = engine_factory_t::create_engine (calling_thread_,
            handler_thread_, iface_, local_object_, object_, engine_arguments_);

        //  Write it into object repository.
        object_info_t info = {handler_thread_, engine};
        it = objects [type_id_].insert (
            objects_t::value_type (object_, info)).first;
    }

    *thread_ = it->second.thread;
    *engine_ = it->second.engine;

    //  Leave critical section.
    sync.unlock ();

    return true;
}
