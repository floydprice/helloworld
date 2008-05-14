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

#include <arpa/inet.h>

#include "locator.hpp"
#include "dispatcher.hpp"
#include "bp_listener.hpp"
#include "bp_engine.hpp"

zmq::locator_t::locator_t (dispatcher_t *dispatcher_, const char *address_,
      uint16_t port_) :
    dispatcher (dispatcher_),
    global_locator (false, address_, port_)
{
    int rc = pthread_mutex_init (&sync, NULL);
    errno_assert (rc == 0);
}

zmq::locator_t::~locator_t ()
{
    int rc = pthread_mutex_destroy (&sync);
    errno_assert (rc == 0);
}

void zmq::locator_t::create_exchange (const char *exchange_,
    i_context *context_, i_engine *engine_, scope_t scope_,
    const char *address_, uint16_t port_, poll_thread_t *listener_thread_,
    int handler_thread_count_, poll_thread_t **handler_threads_)
{
    assert (strlen (exchange_) < 256);
    assert (strlen (address_) < 256);

    //  Enter critical section
    int rc = pthread_mutex_lock (&sync);
    errno_assert (rc == 0);

    //  Add the exchange to the list of known exchanges
    exchange_info_t info = {context_, engine_};
    exchanges.insert (exchanges_t::value_type (exchange_, info));

    //  Add exchange to the global locator
    if (scope_ == scope_global) {

         //  Create a listener for the exchange
         bp_listener_t::create (listener_thread_, address_, port_,
            handler_thread_count_, handler_threads_,
            false, context_, engine_, exchange_);

         //  Send to 'add exchange' command
         unsigned char cmd = add_exchange_id;
         global_locator.blocking_write (&cmd, 1);
         unsigned char size = strlen (exchange_);
         global_locator.blocking_write (&size, 1);
         global_locator.blocking_write (exchange_, size);
         size = strlen (address_);
         global_locator.blocking_write (&size, 1);
         global_locator.blocking_write (address_, size);
         uint16_t port = htons (port_);
         global_locator.blocking_write (&port, 2);
    }

    //  Leave critical section
    rc = pthread_mutex_unlock (&sync);
    errno_assert (rc == 0);
}

void zmq::locator_t::get_exchange (const char *exchange_, i_context **context_,
    i_engine **engine_, poll_thread_t *thread_)
{
    //  Enter critical section
    int rc = pthread_mutex_lock (&sync);
    errno_assert (rc == 0);

    //  Find the exchange
    exchanges_t::iterator it = exchanges.find (exchange_);

    //  If the exchange is unknown, find it using global locator
    if (it == exchanges.end ())
    {

         //  Send to 'get exchange' command
         unsigned char cmd = get_exchange_id;
         global_locator.blocking_write (&cmd, 1);
         unsigned char size = strlen (exchange_);
         global_locator.blocking_write (&size, 1);
         global_locator.blocking_write (exchange_, size);

         //  Read the response
         global_locator.blocking_read (&size, 1);
         char address [256];
         global_locator.blocking_read (address, size);
         address [size] = 0;
         uint16_t port;
         global_locator.blocking_read (&port, 2);
         port = ntohs (port);

         //  Create the proxy engine for the exchange
         bp_engine_t *engine = bp_engine_t::create (thread_,
             false, address, port, 8192, 8192);

         //  Write it into exchange repository
         exchange_info_t info = {thread_, engine};
         it = exchanges.insert (
             exchanges_t::value_type (exchange_, info)).first;
    }

    *context_ = it->second.context;
    *engine_ = it->second.engine;

    //  Leave critical section
    rc = pthread_mutex_unlock (&sync);
    errno_assert (rc == 0);
}

void zmq::locator_t::create_queue (const char *queue_, i_context *context_,
    i_engine *engine_, scope_t scope_, const char *address_, uint16_t port_,
    poll_thread_t *listener_thread_, int handler_thread_count_,
    poll_thread_t **handler_threads_)
{
    //  Enter critical section
    int rc = pthread_mutex_lock (&sync);
    errno_assert (rc == 0);

    queue_info_t info = {context_, engine_};
    queues.insert (queues_t::value_type (queue_, info));

    //  Add queue to the global locator
    if (scope_ == scope_global) {

         //  Send to 'add queue' command
         unsigned char cmd = add_queue_id;
         global_locator.blocking_write (&cmd, 1);
         unsigned char size = strlen (queue_);
         global_locator.blocking_write (&size, 1);
         global_locator.blocking_write (queue_, size);
         size = strlen (address_);
         global_locator.blocking_write (&size, 1);
         global_locator.blocking_write (address_, size);
         uint16_t port = htons (port_);
         global_locator.blocking_write (&port, 2);
    }

    //  Leave critical section
    rc = pthread_mutex_unlock (&sync);
    errno_assert (rc == 0);
}

void zmq::locator_t::get_queue (const char *queue_, i_context **context_,
    i_engine **engine_, poll_thread_t *thread_)
{
    //  Enter critical section
    int rc = pthread_mutex_lock (&sync);
    errno_assert (rc == 0);

    queues_t::iterator it = queues.find (queue_);
    assert (it != queues.end ());
    *context_ = it->second.context;
    *engine_ = it->second.engine;

    //  Leave critical section
    rc = pthread_mutex_unlock (&sync);
    errno_assert (rc == 0);
}
