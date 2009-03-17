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

#ifndef __ZMQ_API_THREAD_HPP_INCLUDED__
#define __ZMQ_API_THREAD_HPP_INCLUDED__

#include <vector>
#include <string>
#include <utility>

#include <zmq/stdint.hpp>
#include <zmq/export.hpp>
#include <zmq/i_thread.hpp>
#include <zmq/i_locator.hpp>
#include <zmq/i_poller.hpp>
#include <zmq/message.hpp>
#include <zmq/dispatcher.hpp>
#include <zmq/ypollset.hpp>
#include <zmq/scope.hpp>
#include <zmq/in_engine.hpp>
#include <zmq/out_engine.hpp>

//  If the RDTSC is available we use it to prevent excessive
//  polling for commands. The nice thing here is that it will work on any
//  system with x86 architecture and gcc or MSVC compiler.
#if (defined __GNUC__ && (defined __i386__ || defined __x86_64__)) ||\
    (defined _MSC_VER && (defined _M_IX86 || defined _M_X64))
#define ZMQ_HAVE_RDTSC_IN_API_THREAD
#endif

namespace zmq
{

    //  Different styles of routing the messages.
    enum style_t
    {
        style_data_distribution = 1,
        style_load_balancing = 2
    };

    enum
    {
        no_limit = -1,
        no_swap = 0
    };

    //  Thread object to be used as a proxy for client application thread.
    //  It is not thread-safe. In case you want to use 0MQ from several
    //  client threads create an api_thread for each of them.

    class api_thread_t : private i_thread
    {
    public:

        //  Creates API thread and attaches it to the command dispatcher and
        //  resource locator.
        ZMQ_EXPORT static api_thread_t *create (dispatcher_t *dispatcher_,
            i_locator *locator_);

        //  Allows user to specify which notifications to receive.
        //  Use binary OR operator to combine individual notification types into
        //  the message mask. Reception of data cannot be switched off.
        //  By default only data are received. 
        ZMQ_EXPORT void mask (uint32_t notifications_);

        //  Creates new exchange, returns exchange ID.
        ZMQ_EXPORT int create_exchange (
            const char *name_, scope_t scope_ = scope_local,
            const char *location_ = NULL, i_thread *listener_thread_ = NULL,
            int handler_thread_count_ = 0, i_thread **handler_threads_ = NULL,
            style_t style_ = style_data_distribution);

        //  Creates new queue, returns queue ID.
        ZMQ_EXPORT int create_queue (
            const char *name_, scope_t scope_ = scope_local,
            const char *location_ = NULL, i_thread *listener_thread_ = NULL,
            int handler_thread_count_ = 0, i_thread **handler_threads_ = NULL,
            int64_t hwm_ = no_limit, int64_t lwm_ = no_limit,
            uint64_t swap_ = no_swap);

        //  Binds an exchange to a queue.
        ZMQ_EXPORT void bind (const char *exchange_name_,
            const char *queue_name_, i_thread *exchange_thread_,
            i_thread *queue_thread_, const char *exchange_options_ = NULL,
            const char *queue_options_ = NULL);

        //  Send a message to specified exchange. 0MQ takes responsibility
        //  for deallocating the message. If there are any pending pre-sent
        //  messages, flush them immediately. If 'block' parameter is true
        //  and there are no queues bound to the exchange, execution will be
        //  blocked until a binding is created. Returns true is message was
        //  successfully enqueued, false if it was not send because fo pipe
        //  limits.
        ZMQ_EXPORT bool send (int exchange_, message_t &message_,
            bool block_ = true);

        //  Presend the message. The message will be stored internally and
        //  sent only after 'flush' is called. In other respects it behaves
        //  the same as 'send' function.
        ZMQ_EXPORT bool presend (int exchange_, message_t &message_,
            bool block_ = true);

        //  Flush all the pre-sent messages.
        ZMQ_EXPORT void flush ();

        //  Receive a message. If 'block' argument is true, it'll block till
        //  message arrives. It returns ID of the queue message was retrieved
        //  from, 0 is no message was retrieved.
        ZMQ_EXPORT int receive (message_t *message_, bool block_ = true);

    private:

        api_thread_t (dispatcher_t *dispatcher_, i_locator *locator_);
        ~api_thread_t ();

        //  i_thread implementation.
        dispatcher_t *get_dispatcher ();
        int get_thread_id ();
        void send_command (i_thread *destination_, const command_t &command_);
        void stop ();
        void destroy ();

        //  Processes single command.
        void process_command (const command_t &command_);

        //  Checks for signals and processes available commands.
        void process_commands ();

        //  Processes available commands. Signals are supplied by the caller.
        void process_commands (ypollset_t::integer_t signals_);

        //  Determines when we are going to poll in 'receive' function. See
        //  api_thread_poll_rate's description in config.hpp to get better
        //  understanding of the algorithm used.
        int ticks;

        //  Stored pointer to the dispatcher.
        dispatcher_t *dispatcher;

        //  Stored pointer to the locator.
        i_locator *locator;

        //  Thread ID assigned to this thread by dispatcher.
        int thread_id;

        //  Used to poll for signals coming from other threads.
        ypollset_t pollset;

        //  List of exchanges belonging to the API thread.
        typedef std::vector <std::pair <std::string, out_engine_t*> >
            exchanges_t;
        exchanges_t exchanges;

        //  List of queues belonging to the API thread.
        typedef std::vector <std::pair <std::string, in_engine_t*> >
            queues_t;
        queues_t queues;

        //  Current queue points to the queue to be used to retrieving
        //  next message.
        queues_t::size_type current_queue; 

        //  Filter specifying what types of messages are wanted by user.
        //  Oher message types are dropped silently.
        uint32_t message_mask;

#if defined ZMQ_HAVE_RDTSC_IN_API_THREAD
        //  Time when last command processing was performed (in ticks).
        //  This optimisation requires RDTSC instruction and is thus
        //  available only on x86 platforms.
        uint64_t last_command_time; 
#endif

        api_thread_t (const api_thread_t&);
        void operator = (const api_thread_t&);
    };

}

#endif
