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

#include <zmq/tcp_listener.hpp>
#include <zmq/platform.hpp>
#include <zmq/err.hpp>
#include <zmq/ip.hpp>
#include <zmq/formatting.hpp>
#include <zmq/fd.hpp>

#include <assert.h>
#include <string.h>
#include <string>
#include <sstream>

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

#ifdef ZMQ_HAVE_WINDOWS

zmq::tcp_listener_t::tcp_listener_t (const char *iface_, bool block)
{
    //  Convert the hostname into sockaddr_in structure.
    sockaddr_in ip_address;
    resolve_ip_interface (&ip_address, iface_);
    
    //  Create a listening socket.
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    wsa_assert (s != INVALID_SOCKET);

    //  Allow reusing of the address.
    int flag = 1;
    int rc = setsockopt (s, SOL_SOCKET, SO_REUSEADDR,
        (const char*) &flag, sizeof (int));
    wsa_assert (rc != SOCKET_ERROR);

    if (!block) {
    
        //  Set non-blocking flag.
        flag = 1;
        rc = ioctlsocket (s, FIONBIO, (u_long *) &flag);
        wsa_assert (rc != SOCKET_ERROR);
    }

    //  Bind the socket to the network interface_i and port.
    rc = bind (s, (struct sockaddr*) &ip_address, sizeof (ip_address));
    wsa_assert (rc != SOCKET_ERROR);

    //  If port number was not specified, retrieve the one assigned
    //  to the socket by the operating system.
    if (ntohs (ip_address.sin_port) == 0) {
        sockaddr_in addr;
        memset (&addr, 0, sizeof (sockaddr_in));
        socklen_t sz = sizeof (sockaddr_in);
        int rc = getsockname (s, (sockaddr*) &addr, &sz);
        wsa_assert (rc != SOCKET_ERROR);
        ip_address.sin_port = addr.sin_port;
    }

    //  Fill in the interface name and port.
    //  TODO: This string should be stored in the locator rather than here.
    if (ip_address.sin_addr.s_addr == htonl (INADDR_ANY)) {
        rc = gethostname (iface, sizeof (iface));
        wsa_assert (rc != SOCKET_ERROR);
    }
    else
        zmq_strncpy (iface, inet_ntoa (ip_address.sin_addr), sizeof (iface));

    std::string port;
    std::stringstream out;
    out << ntohs (ip_address.sin_port);
    port = out.str ();
    zmq_strcat (iface, ":");
    zmq_strcat (iface, port.c_str ());
                  
    //  Listen for incomming connections.
    rc = listen (s, 1);
    wsa_assert (rc != SOCKET_ERROR);
}

zmq::tcp_listener_t::~tcp_listener_t ()
{
    close ();
}

zmq::fd_t zmq::tcp_listener_t::accept ()
{
    //  Accept one incoming connection.
    fd_t sock = ::accept (s, NULL, NULL);
    if (sock == INVALID_SOCKET && 
        (WSAGetLastError () == WSAEWOULDBLOCK || WSAGetLastError () == WSAECONNRESET))
        return retired_fd;

    wsa_assert (sock != INVALID_SOCKET);
    return sock;
}

void zmq::tcp_listener_t::close ()
{
    assert (s != retired_fd);
    int rc = closesocket (s);
    wsa_assert (rc != SOCKET_ERROR);
    s = retired_fd;
}

#else

zmq::tcp_listener_t::tcp_listener_t (const char *iface_, bool block)
{
    //  Convert the hostname into sockaddr_in structure.
    sockaddr_in ip_address;
    resolve_ip_interface (&ip_address, iface_);
    
    //  Create a listening socket.
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    errno_assert (s != -1);

    //  Allow reusing of the address.
    int flag = 1;
    int rc = setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
    errno_assert (rc == 0);

    if (!block) {
    
        //  Set non-blocking flag.
        flag = fcntl (s, F_GETFL, 0);
        if (flag == -1) 
            flag = 0;
        rc = fcntl (s, F_SETFL, flag | O_NONBLOCK);
        errno_assert (rc != -1);
    }

    //  Bind the socket to the network interface_i and port.
    rc = bind (s, (struct sockaddr*) &ip_address, sizeof (ip_address));
    errno_assert (rc == 0);

    //  If port number was not specified, retrieve the one assigned
    //  to the socket by the operating system.
    if (ntohs (ip_address.sin_port) == 0) {
        sockaddr_in addr;
        memset (&addr, 0, sizeof (sockaddr_in));
        socklen_t sz = sizeof (sockaddr_in);
        int rc = getsockname (s, (sockaddr*) &addr, &sz);
        errno_assert (rc == 0);
        ip_address.sin_port = addr.sin_port;
    }

    //  Fill in the interface name and port.
    //  TODO: This string should be stored in the locator rather than here.
    const char *rcp;
    size_t isz;
    if (ip_address.sin_addr.s_addr == htonl (INADDR_ANY)) {
        rc = gethostname (iface, sizeof (iface));
        assert (rc == 0);
        isz = strlen (iface);
    }
    else {
        rcp = inet_ntop (AF_INET, &ip_address.sin_addr, iface, sizeof (iface));
        assert (rcp);
        isz = strlen (iface);
    }
    zmq_snprintf (iface + isz, sizeof (iface) - isz, ":%d",
        (int) ntohs (ip_address.sin_port));
              
    //  Listen for incomming connections.
    rc = listen (s, 10);
    errno_assert (rc == 0);
}

zmq::tcp_listener_t::~tcp_listener_t ()
{
    close ();
}

zmq::fd_t zmq::tcp_listener_t::accept ()
{
    //  Accept one incoming connection.
    fd_t sock = ::accept (s, NULL, NULL);

#if defined ZMQ_HAVE_QNXNTO 
    if (sock == -1 && 
        (errno == EWOULDBLOCK || errno == EINTR || errno == ECONNABORTED))
        return retired_fd;
#elif defined ZMQ_HAVE_HPUX
    if (sock == -1 && 
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || 
         errno == ECONNABORTED || errno == ENOBUFS))
        return retired_fd;
#elif defined ZMQ_HAVE_SOLARIS || defined ZMQ_HAVE_AIX
    if (sock == -1 && 
        (errno == EWOULDBLOCK || errno == EINTR || errno == ECONNABORTED || errno == EPROTO))
        return retired_fd;
#elif defined ZMQ_HAVE_LINUX || defined ZMQ_HAVE_FREEBSD || \
      defined ZMQ_HAVE_OPENBSD || defined ZMQ_HAVE_OSX 
    if (sock == -1 && 
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ECONNABORTED))
        return retired_fd;
#endif

    errno_assert (sock != -1); 
    return sock;
}

void zmq::tcp_listener_t::close ()
{
    assert (s != retired_fd);
    int rc = ::close (s);
    errno_assert (rc == 0);
    s = retired_fd;
}

#endif
