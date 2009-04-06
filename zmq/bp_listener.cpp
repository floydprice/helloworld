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

#include "bp_listener.hpp"
#include "bp_engine.hpp"
#include "config.hpp"

zmq::bp_listener_t *zmq::bp_listener_t::create (poll_thread_t *thread_,
    const char *interface_, int handler_thread_count_,
    poll_thread_t **handler_threads_, bool source_,
    i_context *peer_context_, i_engine *peer_engine_,
    const char *peer_name_, int hwm_, int lwm_, const char *queue_name_,
    uint64_t swap_size_)
{
    bp_listener_t *instance = new bp_listener_t (thread_, interface_,
        handler_thread_count_, handler_threads_, source_, peer_context_,
        peer_engine_, peer_name_, hwm_, lwm_, queue_name_, swap_size_);
    assert (instance);

    return instance;
}

zmq::bp_listener_t::bp_listener_t (poll_thread_t *thread_,
      const char *interface_, int handler_thread_count_,
      poll_thread_t **handler_threads_, bool source_,
      i_context *peer_context_, i_engine *peer_engine_,
      const char *peer_name_, int hwm_, int lwm_, const char *queue_name_,
      uint64_t swap_size_) :
    source (source_),
    context (thread_),
    peer_context (peer_context_),
    peer_engine (peer_engine_),
    listener (interface_),
    hwm (hwm_),
    lwm (lwm_),
    size_monitoring (source_),
    swap_size (swap_size_)
{
    //  Copy the peer name.
    assert (strlen (peer_name_) < 16);
    strcpy (peer_name, peer_name_);

    //  If we are to monitor the queue size, let's store its name.
    if (size_monitoring && queue_name_)
        queue_name = queue_name_;

    //  Initialise the array of threads to handle new connections.
    assert (handler_thread_count_ > 0);
    for (int thread_nbr = 0; thread_nbr != handler_thread_count_; thread_nbr ++)
        handler_threads.push_back (handler_threads_ [thread_nbr]);
    current_handler_thread = 0;

    //  Register the listener with the polling thread.
    thread_->register_engine (this);   
}

zmq::bp_listener_t::~bp_listener_t ()
{
}

void zmq::bp_listener_t::set_poller (i_poller *poller_, int handle_)
{
    poller_->set_fd (handle_, listener.get_fd ());
    poller_->set_pollin (handle_);
}

bool zmq::bp_listener_t::in_event ()
{
    //  Accept incoming TCP connection.
    tcp_socket_t *socket = new tcp_socket_t (listener);
    assert (socket);

    //  Fetch remote peer's ID.
    std::string remote_id = socket->recv_string (256);

    engines_t::iterator it = engines.find (remote_id);
    if (it != engines.end ()) {
        command_t cmd;
        cmd.init_engine_reconnect (it->second, socket);
        context->send_command (it->second->get_context (), cmd);
        return true;
    }

    //  Create the engine to take care of the connection.
    //  TODO: make buffer size configurable by user
    bp_engine_t *engine = bp_engine_t::create (
        handler_threads [current_handler_thread], socket,
        bp_out_batch_size, bp_in_batch_size, peer_name);
    assert (engine);

    //  All engines created by TCP listener are kept in his private dictionary.
    engines [remote_id] = engine;

    if (source) {

        //  The newly created engine serves as a local source of messages
        //  I.e. it reads messages from the socket and passes them on to
        //  the peer engine.
        i_context *source_context = handler_threads [current_handler_thread];
        i_engine *source_engine = engine;

        //  Create the pipe to the newly created engine.
        pipe_t *pipe = new pipe_t (source_context, source_engine,
            peer_context, peer_engine, hwm, lwm, queue_name.c_str (),
            swap_size);
        assert (pipe);

        //  Bind new engine to the source end of the pipe.
        command_t cmd_send_to;
        cmd_send_to.init_engine_send_to (source_engine, "", pipe);
        context->send_command (source_context, cmd_send_to);

        //  Bind the peer to the destination end of the pipe.
        command_t cmd_receive_from;
        cmd_receive_from.init_engine_receive_from (peer_engine,
            peer_name, pipe);
        context->send_command (peer_context, cmd_receive_from);
    }
    else {

        //  The newly created engine serves as a local destination of messages
        //  I.e. it sends messages received from the peer engine to the socket.
        i_context *destination_context =
            handler_threads [current_handler_thread];
        i_engine *destination_engine = engine;

        //  Create the pipe to the newly created engine.
        pipe_t *pipe = new pipe_t (peer_context, peer_engine,
            destination_context, destination_engine, hwm, lwm);
        assert (pipe);

        //  Bind new engine to the destination end of the pipe.
        command_t cmd_receive_from;
        cmd_receive_from.init_engine_receive_from (
            destination_engine, "", pipe);
        context->send_command (destination_context, cmd_receive_from);

        //  Bind the peer to the source end of the pipe.
        command_t cmd_send_to;
        cmd_send_to.init_engine_send_to (peer_engine, peer_name, pipe);
        context->send_command (peer_context, cmd_send_to);
    }

    //  Move to the next thread to get round-robin balancing of engines.
    current_handler_thread ++;
    if (current_handler_thread == handler_threads.size ())
        current_handler_thread = 0;
    return true;
}

bool zmq::bp_listener_t::out_event ()
{
    //  We will never get POLLOUT when listening for incoming connections.
    assert (false);
    return true;
}

bool zmq::bp_listener_t::close_event()
{
    //  TODO: engine tear-down
    assert (false);
}

void zmq::bp_listener_t::process_command (const engine_command_t &command_)
{
    //  TODO: The only event handled here should be terminate, which would
    //  release the object (delete this)
    assert (false);
}
