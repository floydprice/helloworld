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

#ifndef __ZMQ_BP_LISTENER_HPP_INCLUDED__
#define __ZMQ_BP_LISTENER_HPP_INCLUDED__

#include <vector>

#include "i_pollable.hpp"
#include "i_thread.hpp"
#include "tcp_listener.hpp"
#include "declspec_export.hpp"

namespace zmq
{

    //  BP (backend protocol) listener. Listens on a specified network
    //  interface and port and creates a BP engine for every new connection.

    class bp_listener_t : public i_pollable
    {
    public:

        //  Creates a BP listener. Handler thread array determines
        //  the threads that will serve newly-created BP engines.
        declspec_export static bp_listener_t *create (
            i_thread *calling_thread_,
            i_thread *thread_, const char *interface_,
            int handler_thread_count_, i_thread **handler_threads_,
            bool source_, i_thread *peer_thread_, i_engine *peer_engine_,
            const char *peer_name_);

        //  Returns port listener is listening on.
        declspec_export inline const char *get_interface ()
        {
            return listener.get_interface ();
        }

        //  i_pollable implementation.
        declspec_export void register_event (i_poller *poller_);
        declspec_export void in_event ();
        declspec_export void out_event ();
        declspec_export void error_event ();
        declspec_export void unregister_event ();
        declspec_export void process_command (
            const engine_command_t &command_);

    private:

        bp_listener_t (i_thread *calling_thread_, i_thread *thread_,
            const char *interface_, int handler_thread_count_,
            i_thread **handler_threads_, bool source_,
            i_thread *peer_thread_, i_engine *peer_engine_,
            const char *peer_name_);
        ~bp_listener_t ();

        //  Determines whether the engine serves as a local source of messages
        //  (i.e. reads them from the sockets and makes them available) or
        //  a local destination of messages (i.e. gathers the messages and
        //  sends them to the socket).
        bool source;

        //  The thread listener is running in.
        i_thread *thread;

        //  Determine the engine and the object (either exchange or queue)
        //  within the engine to serve as a peer to this engine.
        i_thread *peer_thread;
        i_engine *peer_engine;
        char peer_name [16];

        //  Listening socket.
        tcp_listener_t listener;

        //  The thread array to manage newly-created BP engines.
        typedef std::vector <i_thread*> handler_threads_t;
        handler_threads_t handler_threads;

        //  Points to the I/O thread to use to handle next BP connection.
        //  (Handler threads are used in round-robin fashion.)
        handler_threads_t::size_type current_handler_thread;

        bp_listener_t (const bp_listener_t&);
        void operator = (const bp_listener_t&);
    };

}

#endif
