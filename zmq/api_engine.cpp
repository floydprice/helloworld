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

#include "api_engine.hpp"

zmq::api_engine_t::api_engine_t (dispatcher_t *dispatcher_) :
    ticks (0),
    dispatcher (dispatcher_),
    current_queue (queues.begin ())
{
    //  Register the thread with the command dispatcher
    thread_id = dispatcher->allocate_thread_id (&pollset);
}

zmq::api_engine_t::~api_engine_t ()
{
    //  Unregister the thread from the command dispatcher
    dispatcher->deallocate_thread_id (thread_id);
}

void zmq::api_engine_t::create_exchange (const char *exchange_,
    scope_t scope_, const char *address_, uint16_t port_,
    poll_thread_t *listener_thread_, int handler_thread_count_,
    poll_thread_t **handler_threads_)
{
#ifdef ZMQ_DEBUG
    printf ("Creating exchange %s.\n", exchange_);
#endif
    //  Insert the exchange to the local list of exchanges
    //  If the exchange is already present, return immediately
    if (!exchanges.insert (
          exchanges_t::value_type (exchange_, demux_t ())).second)
        return;

    if (scope_ == scope_local)
        return;

    //  Register the exchange with the locator
    dispatcher->get_locator ().create_exchange (exchange_, this, this,
        scope_, address_, port_, listener_thread_,
        handler_thread_count_, handler_threads_);
}

void zmq::api_engine_t::create_queue (const char *queue_, scope_t scope_,
    const char *address_, uint16_t port_, poll_thread_t *listener_thread_,
    int handler_thread_count_, poll_thread_t **handler_threads_)
{
#ifdef ZMQ_DEBUG
    printf ("Creating queue %s.\n", queue_);
#endif
    //  Insert the queue to the local list of queues
    //  If the queue is already present, return immediately
    if (!queues.insert (
          queues_t::value_type (queue_, mux_t ())).second)
        return;

    //  Move the current queue pointer to the beginning of the list
    current_queue = queues.begin ();

    if (scope_ == scope_local)
        return;

    //  Register the queue with the locator
    dispatcher->get_locator ().create_queue (queue_, this, this, scope_,
        address_, port_, listener_thread_, handler_thread_count_,
        handler_threads_);
}

void zmq::api_engine_t::bind (const char *exchange_, const char *queue_,
    poll_thread_t *exchange_thread_, poll_thread_t *queue_thread_)
{
#ifdef ZMQ_DEBUG
    printf ("Binding exchange %s to queue %s.\n", exchange_, queue_);
#endif
    //  Find the exchange
    i_context *exchange_context;
    i_engine *exchange_engine;
    exchanges_t::iterator eit = exchanges.find (exchange_);
    if (eit != exchanges.end ()) {
        exchange_context = this;
        exchange_engine = this;
    }
    else {
        dispatcher->get_locator ().get_exchange (exchange_,
            &exchange_context, &exchange_engine, exchange_thread_);
    }

    //  Find the queue
    i_context *queue_context;
    i_engine *queue_engine;
    queues_t::iterator qit = queues.find (queue_);
    if (qit != queues.end ()) {
        queue_context = this;
        queue_engine = this;
    }
    else {
        dispatcher->get_locator ().get_queue (queue_,
            &queue_context, &queue_engine, queue_thread_);
    }

    //  Create the pipe
    pipe_t *pipe = new pipe_t (exchange_context, exchange_engine,
        queue_context, queue_engine);
    assert (pipe);

    //  Bind the source end of the pipe
    if (eit != exchanges.end ())
        eit->second.send_to (pipe);
    else {
        command_t cmd;
        cmd.init_engine_send_to (exchange_engine, exchange_, pipe);
        send_command (exchange_context, cmd);
    }

    //  Bind the destination end of the pipe
    if (qit != queues.end ())
        qit->second.receive_from (pipe);
    else {
        command_t cmd;
        cmd.init_engine_receive_from (queue_engine, queue_, pipe);
        send_command (queue_context, cmd);
    }
}

void zmq::api_engine_t::send (const char *exchange_, void *value_)
{
#ifdef ZMQ_DEBUG
    printf ("Sending message to exchange %s.\n", exchange_);
#endif
    //  Check the signals and process the commands if there are any
    ypollset_t::integer_t signals = pollset.check ();
    if (signals)
        process_commands (signals);

    //  Find the appropriate exchange
    exchanges_t::iterator it = exchanges.find (exchange_);
    assert (it != exchanges.end ());

    //  Pass the message to the demux
    it->second.instant_write (value_);
}

void *zmq::api_engine_t::receive (bool block_)
{
#ifdef ZMQ_DEBUG
    printf ("Retrieveing a message.\n");
#endif
    void *msg = NULL;

    //  Remember the original current queue position. We'll use this value
    //  as a marker for identifying whether we've inspected all the queues.
    queues_t::iterator start = current_queue;

    //  Big loop - iteration happens each time after new commands
    //  are processed (thus new messages may be available)
    while (true) {

        //  Small loop doesn't make sense if there are no queues
        if (!queues.empty ()) {

            //  Small loop - we are iterating over the array of queues
            //  checking whether any of them has messages available
            while (true) {

               //  Get a message
               msg = current_queue->second.read ();

               //  Move to the next queue
               current_queue ++;
               if (current_queue == queues.end ())
                   current_queue = queues.begin ();

               //  If we have a message skip the small loop
               if (msg)
                   break;

               //  If we've iterated over all the queues skip the loop
               if (current_queue == start)
                   break;
            }
        }

        //  If we have a message skip the big loop
        if (msg)
            break;

        //  If we don't have a message and no blocking is required
        //  skip the big loop
        if (!block_)
            break;

        //  This is a blocking call and we have no messages
        //  We wait for commands, we process them and we continue
        //  with getting the messages.
        ypollset_t::integer_t signals = pollset.poll ();
        assert (signals);
        process_commands (signals);
        ticks = 0;
    }

    //  Once every max_ticks messages check for signals and process incoming
    //  commands. This happens only if we are not polling altogether because
    //  there are messages available all the time. If poll occurs, ticks is
    //  set to zero and thus we avoid this code.
    if (++ ticks == max_ticks) {
        ypollset_t::integer_t signals = pollset.check ();
        if (signals)
            process_commands (signals);
        ticks = 0;
    }

    return msg;
}

int zmq::api_engine_t::get_thread_id ()
{
    return thread_id;
}

void zmq::api_engine_t::send_command (i_context *destination_,
    const command_t &command_)
{
    dispatcher->write (thread_id, destination_->get_thread_id (), command_);
}

void zmq::api_engine_t::process_command (const engine_command_t &command_)
{
    switch (command_.type) {
    case engine_command_t::revive:

        //  Forward the revive command to the pipe
        command_.args.revive.pipe->revive ();
        break;

    case engine_command_t::send_to:

        {
            //  Find the right demux
            exchanges_t::iterator it =
                exchanges.find (command_.args.send_to.exchange);
            assert (it != exchanges.end ());

            //  Start sending messages to a pipe
            it->second.send_to (command_.args.send_to.pipe);
        }
        break;

    case engine_command_t::receive_from:

        {
            //  Find the right mux
            queues_t::iterator it = 
                queues.find (command_.args.receive_from.queue);
            assert (it != queues.end ());

            //  Start receiving messages from a pipe
            it->second.receive_from (command_.args.receive_from.pipe);
        }
        break;

    default:

        //  Unsupported/unknown command
        assert (false);
     }
}

void zmq::api_engine_t::process_commands (ypollset_t::integer_t signals_)
{
    for (int source_thread_id = 0;
          source_thread_id != dispatcher->get_thread_count ();
          source_thread_id ++) {
        if (signals_ & (1 << source_thread_id)) {
            dispatcher_t::item_t *first;
            dispatcher_t::item_t *last;
            dispatcher->read (source_thread_id, thread_id, &first, &last);
            while (first != last) {
                switch (first->value.type) {

                //  Process engine command
                case command_t::engine_command:
                    assert (first->value.args.engine_command.engine ==
                        (i_engine*) this);
                    process_command (
                        first->value.args.engine_command.command);
                    break;

                //  Unsupported/unknown command
                default:
                    assert (false);
                }
                dispatcher_t::item_t *o = first;
                first = first->next;
                delete o;
            }
        }
    }
}
