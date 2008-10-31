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

#include "tcp_socket.hpp"

#include <assert.h>
#include <string.h>

#include "platform.hpp"
#ifdef ZMQ_HAVE_WINDOWS
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "err.hpp"
#include "ip.hpp"

#ifdef ZMQ_HAVE_WINDOWS

zmq::tcp_socket_t::tcp_socket_t (const char *hostname_, bool block_) :
    block (block_)
{
    //  Convert the hostname into sockaddr_in structure.
    sockaddr_in ip_address;
    resolve_ip_hostname (&ip_address, hostname_);

    //  Create the socket.
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    wsa_assert (s != SOCKET_ERROR);

    
    //  Connect to the remote peer.
    int rc = connect (s, (sockaddr *)&ip_address, sizeof (ip_address));
    wsa_assert (rc != SOCKET_ERROR);
 
    //  Set socket properties to non-blocking mode. 
    if (! block) {
        unsigned long argp = 1;
        rc = ioctlsocket (s, FIONBIO, &argp);
        wsa_assert (rc != SOCKET_ERROR);
    }
 	
    //  Disable Nagle's algorithm.
    int flag = 1;
    rc = setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof (int));
    wsa_assert (rc != SOCKET_ERROR);
}

zmq::tcp_socket_t::tcp_socket_t (tcp_listener_t &listener, bool block_):
    block (block_)
{
    //  Accept the socket.
    s = listener.accept ();
 
    //  Set socket properties to non-blocking mode. 
    if (! block) {
        unsigned long argp = 1;
        int rc = ioctlsocket (s, FIONBIO, &argp);
        wsa_assert (rc != SOCKET_ERROR);
    }

    //  Disable Nagle's algorithm.
    int flag = 1;
    int rc = setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
        sizeof (int));
    wsa_assert (rc != SOCKET_ERROR);
}

zmq::tcp_socket_t::~tcp_socket_t ()
{
    int rc = closesocket(s);
    wsa_assert (rc != SOCKET_ERROR);
    
}

int zmq::tcp_socket_t::write (const void *data, int size)
{
    int nbytes = send (s, (char*) data, size, 0);
		
    //  Signalise peer failure.
    if (nbytes == SOCKET_ERROR)
        return -1;

    // TODO: check error
    return (size_t) nbytes;
}

int zmq::tcp_socket_t::read (void *data, int size)
{
    int nbytes = recv (s, (char*) data, size, 0);
    if (nbytes == SOCKET_ERROR)
        return -1;
    return (size_t) nbytes;
}

#else

zmq::tcp_socket_t::tcp_socket_t (const char *hostname_, bool block_) :
    block (block_)
{
    //  Convert the hostname into sockaddr_in structure.
    sockaddr_in ip_address;
    resolve_ip_hostname (&ip_address, hostname_);

    //  Create the socket.
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    errno_assert (s != -1);

    //  Connect to the remote peer.
    int rc = connect (s, (sockaddr *)&ip_address, sizeof (ip_address));
    errno_assert (rc != -1);

    //  Disable Nagle's algorithm.
    int flag = 1;
    rc = setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof (int));
    errno_assert (rc == 0);
}

zmq::tcp_socket_t::tcp_socket_t (tcp_listener_t &listener, bool block_) :
    block (block_)
{
    //  Accept the socket.
    s = listener.accept ();

    //  Disable Nagle's algorithm.
    int flag = 1;
    int rc = setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
        sizeof (int));
    errno_assert (rc == 0);
}

zmq::tcp_socket_t::~tcp_socket_t ()
{
    int rc = close (s);
    errno_assert (rc == 0);
}

int zmq::tcp_socket_t::write (const void *data, int size)
{
    ssize_t nbytes = send (s, data, size, block ? MSG_DONTWAIT : 0);

    //  If not a single byte can be written to the socket in non-blocking mode
    //  we'll get an error (this may happen during the speculative write).
    if (nbytes == -1 && errno == EAGAIN)
        return 0;

    //  Signalise peer failure.
    if (nbytes == -1 && errno == ECONNRESET)
        return -1;

    errno_assert (nbytes != -1);
    return (size_t) nbytes;
}

int zmq::tcp_socket_t::read (void *data, int size)
{
    ssize_t nbytes = recv (s, data, size, block ? 0 : MSG_DONTWAIT);

    //  If not a single byte can be read from the socket in non-blocking mode
    //  we'll get an error (this may happen during the speculative read).
    if (nbytes == -1 && errno == EAGAIN)
        return 0;

    //  Orderly shutdown by the other peer.
    if (nbytes == 0)
        return -1;

    errno_assert (nbytes != -1);
    return (size_t) nbytes;
}

#endif
