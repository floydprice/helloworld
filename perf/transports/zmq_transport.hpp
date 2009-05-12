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

#ifndef __PERF_ZMQ_TRANSPORT_HPP_INCLUDED__
#define __PERF_ZMQ_TRANSPORT_HPP_INCLUDED__

#include <stdio.h>

#include "i_transport.hpp"

#include <zmq.hpp>

namespace perf
{
    bool error_handler (const char *local_, const char *remote_)
    {
        printf ("connection %s-%s broken\n", local_, remote_);
        return false;
    }

    class zmq_t : public i_transport
    {
    public:
        zmq_t (bool bind_, const char *exchange_name_,
              const char *queue_name_, const char *exchange_interface_,
              const char *queue_interface_) :
            dispatcher (2)
        {

            api = zmq::api_thread_t::create (&dispatcher, &locator);
            worker = zmq::io_thread_t::create (&dispatcher);

            if (bind_) {

                //  Create & bind local exchange.
                exchange_id = api->create_exchange ("E_LOCAL");
                api->bind ("E_LOCAL", NULL, queue_name_, queue_interface_,
                    worker, worker);
                
                //  Create & bind local queue.
                api->create_queue ("Q_LOCAL");
                api->bind (exchange_name_, exchange_interface_, "Q_LOCAL", NULL,
                    worker, worker);

            } else {
                assert (exchange_interface_);
                assert (queue_interface_);
                
                api->create_queue (queue_name_, zmq::scope_global,
                    queue_interface_, worker, 1, &worker);

                exchange_id = api->create_exchange (exchange_name_, 
                    zmq::scope_global, exchange_interface_, worker, 
                    1, &worker);
            }
            //  Set error handler function (to ignore disconnected receivers).
            zmq::set_error_handler (error_handler);
          
        }

        inline ~zmq_t ()
        {
#ifdef ZMQ_HAVE_WINDOWS
	    Sleep (1000);
#else
            sleep (1);
#endif
        }

        inline virtual void send (size_t size_)
        {
            zmq::message_t message (size_);
            api->send (exchange_id, message);
        }

        inline virtual size_t receive ()
        {
            zmq::message_t message;
            api->receive (&message);
            return message.size ();
        }

    protected:

        zmq::dispatcher_t dispatcher;
        zmq::locator_t locator;
        zmq::api_thread_t *api;
        zmq::i_thread *worker;
	int exchange_id;
    };

}

#endif
