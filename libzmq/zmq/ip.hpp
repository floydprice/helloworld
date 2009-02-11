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

#ifndef __ZMQ_IP_HPP_INCLUDED__
#define __ZMQ_IP_HPP_INCLUDED__

#include <zmq/platform.hpp>

#ifdef ZMQ_HAVE_WINDOWS
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <Ipexport.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

namespace zmq
{

    //  Resolves network interface name in <ip-address>:<port> format.
    void resolve_ip_interface (sockaddr_in* addr_, char const *interface_);

    //  This function resolves a string in <hostname>:<port-number> format.
    //  Hostname can be either the name of the host or its IP address.
    void resolve_ip_hostname (sockaddr_in *addr_, const char *hostname_);
}

#endif 
