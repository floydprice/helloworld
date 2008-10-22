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

#ifndef __ZMQ_TCP_LISTENER_HPP_INCLUDED__
#define __ZMQ_TCP_LISTENER_HPP_INCLUDED__

#include "stdint.hpp"
#include "declspec_export.hpp"

namespace zmq
{
    //  The class encapsulating simple TCP listening socket.

    class tcp_listener_t
    {
    public:

        //  Create TCP listining socket. Interface is either interface name,
        //  in that case port number is chosen by OS and can be retrieved
        //  by get_port method, or <interface-name>:<port-number>.
        declspec_export tcp_listener_t (const char *interface_);

        //  Get the file descriptor to poll on to get notified about
        //  newly created connections.
        declspec_export inline int get_fd ()
        {
            return s;
        }

        //  Returns port listener is listening on.
        declspec_export inline const char *get_interface ()
        {
            return iface; 
        }

        //  Accept the new connection.
        declspec_export int accept ();

    private:

        //  Name of the interface listenet is listening on.
        char iface [256];

        //  Underlying socket.
        int s;

        tcp_listener_t (const tcp_listener_t&);
        void operator = (const tcp_listener_t&);
    };

}

#endif
