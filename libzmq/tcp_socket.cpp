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

#include <zmq/tcp_socket.hpp>

#include <string.h>
#include <stdio.h>

#include <zmq/platform.hpp>
#ifdef ZMQ_HAVE_WINDOWS
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include <zmq/err.hpp>
#include <zmq/ip.hpp>

#ifdef ZMQ_HAVE_WINDOWS

zmq::tcp_socket_t::tcp_socket_t (const char *hostname_, bool block_) :
    s (retired_fd),
    hostname (hostname_),
    block (block_)
{
    reopen ();
}

zmq::tcp_socket_t::tcp_socket_t (tcp_listener_t &listener, bool block_) :
    s (retired_fd),
    hostname (""),
    block (block_)
{
    //  Accept the socket.
    s = listener.accept ();
    wsa_assert (s != INVALID_SOCKET);
 
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

#ifdef ZMQ_HAVE_OPENVMS
    //  Disable delayed acknowledgements.
    flag = 1;
    rc = setsockopt (s, IPPROTO_TCP, TCP_NODELACK, (char*) &flag, sizeof (int));
    wsa_assert (rc != SOCKET_ERROR);
#endif
}

zmq::tcp_socket_t::~tcp_socket_t ()
{
	if (s != retired_fd)
		close ();
}

void zmq::tcp_socket_t::close ()
{
    zmq_assert (s != retired_fd);
    int rc = closesocket (s);
    wsa_assert (rc != SOCKET_ERROR);
    s = retired_fd;
}

void zmq::tcp_socket_t::reopen ()
{
    zmq_assert (s == retired_fd);
    zmq_assert (hostname != "");

    //  Convert the hostname into sockaddr_in structure.
    sockaddr_in ip_address;
    resolve_ip_hostname (&ip_address, hostname.c_str ());

    //  Create the socket.
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    wsa_assert (s != INVALID_SOCKET);

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

    //  Connect to the remote peer.
    rc = connect (s, (sockaddr*) &ip_address, sizeof ip_address);

    //  Blocking socket is used only for communication with zmq_server.
    //  Thus it's failure is always a failure to connect to zmq_server.
    if (block && rc == SOCKET_ERROR) {
        fprintf (stderr, "Cannot connect to zmq_server at %s.\n",
            hostname.c_str ());\
        wsa_assert (false);
    }

    if (!(rc == 0 || (rc == SOCKET_ERROR &&
          (WSAGetLastError () == WSAEINPROGRESS ||
          WSAGetLastError () == WSAEWOULDBLOCK)))) {
        close ();
        return;
    }
}

int zmq::tcp_socket_t::write (const void *data, int size)
{
    int nbytes = send (s, (char*) data, size, 0);

    //  If not a single byte can be written to the socket in non-blocking mode
    //  we'll get an error (this may happen during the speculative write).
    if (nbytes == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK)
        return 0;
		
    //  Signalise peer failure.
    if (nbytes == SOCKET_ERROR && WSAGetLastError () == WSAECONNRESET)
        return -1;

    wsa_assert (nbytes != SOCKET_ERROR);

    return (size_t) nbytes;
}

int zmq::tcp_socket_t::read (void *data, int size)
{
    int nbytes = recv (s, (char*) data, size, 0);

    //  If not a single byte can be read from the socket in non-blocking mode
    //  we'll get an error (this may happen during the speculative read).
    if (nbytes == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK)
        return 0;

    //  Connection failure.
    if (nbytes == -1 && (
          WSAGetLastError () == WSAECONNRESET ||
          WSAGetLastError () == WSAECONNREFUSED ||
          WSAGetLastError () == WSAENOTCONN))
        return -1;

    wsa_assert (nbytes != SOCKET_ERROR);

    //  Orderly shutdown by the other peer.
    if (nbytes == 0)
        return -1; 

    return (size_t) nbytes;
}

bool zmq::tcp_socket_t::socket_error ()
{
    int err;

    int len = sizeof err;
    int rc = getsockopt (s, SOL_SOCKET, SO_ERROR, (char*) &err, &len);
    wsa_assert (rc != SOCKET_ERROR);

    return err != 0;
}

#else

zmq::tcp_socket_t::tcp_socket_t (const char *hostname_, bool block_) :
    s (retired_fd),
    hostname (hostname_),
    block (block_)
{
    reopen ();
}

zmq::tcp_socket_t::tcp_socket_t (tcp_listener_t &listener, bool block_) :
    hostname (""),
    block (block_)
{
    //  Accept the socket.
    s = listener.accept ();
    
    if (! block) {

        // Set to non-blocking mode.
        int flags = fcntl (s, F_GETFL, 0);
        if (flags == -1) 
            flags = 0;
        int rc = fcntl (s, F_SETFL, flags | O_NONBLOCK);
        errno_assert (rc != -1);
    }

    //  Disable Nagle's algorithm.
    int flag = 1;
    int rc = setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
        sizeof (int));
    errno_assert (rc == 0);
}

zmq::tcp_socket_t::~tcp_socket_t ()
{
    if (s != retired_fd)
        close ();
}

void zmq::tcp_socket_t::close ()
{
    zmq_assert (s != retired_fd);
    int rc = ::close (s);
    errno_assert (rc == 0);
    s = retired_fd;
}

void zmq::tcp_socket_t::reopen ()
{
    zmq_assert (s == retired_fd);
    zmq_assert (hostname != "");

    //  Convert the hostname into sockaddr_in structure.
    sockaddr_in ip_address;
    resolve_ip_hostname (&ip_address, hostname.c_str ());

    //  Create the socket.
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    errno_assert (s != -1);
      
    if (! block) {

        //  Set to non-blocking mode.
        int flags = fcntl (s, F_GETFL, 0);
        if (flags == -1)
            flags = 0;
        int rc = fcntl (s, F_SETFL, flags | O_NONBLOCK);
        errno_assert (rc != -1);        
    }

    //  Disable Nagle's algorithm.
    int flag = 1;
    int rc = setsockopt (s, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
        sizeof (int));
    errno_assert (rc == 0);

#ifdef ZMQ_HAVE_OPENVMS
    //  Disable delayed acknowledgements.
    flag = 1;
    rc = setsockopt (s, IPPROTO_TCP, TCP_NODELACK, (char*) &flag, sizeof (int));
    wsa_assert (rc != SOCKET_ERROR);
#endif

    //  Connect to the remote peer.
    rc = connect (s, (sockaddr*) &ip_address, sizeof ip_address);

    //  Blocking socket is used only for communication with zmq_server.
    //  Thus it's failure is always a failure to connect to zmq_server.
    if (block && rc != 0) {
        fprintf (stderr, "Cannot connect to zmq_server at %s.\n",
            hostname.c_str ());\
        errno_assert (false);
    }

    if (!(rc == 0 || (rc == -1 && errno == EINPROGRESS)))
        close ();
}

int zmq::tcp_socket_t::write (const void *data, int size)
{
    ssize_t nbytes = send (s, data, size, 0);

    //  If not a single byte can be written to the socket in non-blocking mode
    //  we'll get an error (this may happen during the speculative write).
    if (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK ||
          errno == EINTR))
        return 0;

    //  Signalise peer failure.
    if (nbytes == -1 && errno == ECONNRESET)
        return -1;

    errno_assert (nbytes != -1);
    return (size_t) nbytes;
}

int zmq::tcp_socket_t::read (void *data, int size)
{
    ssize_t nbytes = recv (s, data, size, block ? MSG_WAITALL : 0);

    //  If not a single byte can be read from the socket in non-blocking mode
    //  we'll get an error (this may happen during the speculative read).
    if (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK ||
          errno == EINTR))
        return 0;

    //  Signalise peer failure.
    if (nbytes == -1 && (errno == ECONNRESET || errno == ECONNREFUSED))
        return -1;

    errno_assert (nbytes != -1);

    //  Orderly shutdown by the other peer.
    if (nbytes == 0)
        return -1;

    return (size_t) nbytes;
}

bool zmq::tcp_socket_t::socket_error ()
{
    //  Following code should handle both Berkeley-derived socket
    //  implementations and Solaris.
    int err = 0;
    socklen_t len = sizeof err;
    int rc = getsockopt (s, SOL_SOCKET, SO_ERROR, (char*) &err, &len);
    if (rc == -1)
        err = errno;
    zmq_assert (err == 0 || err == ECONNREFUSED || err == ETIMEDOUT ||
        err == ECONNRESET || err == EADDRNOTAVAIL || err == EHOSTUNREACH);
    return err != 0;
}

#endif

