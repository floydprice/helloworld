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

#include <zmq/err.hpp>
#include <zmq/platform.hpp>

#ifdef ZMQ_HAVE_WINDOWS

#include <winsock2.h>

const char *zmq::wsa_error()
{
    int errcode = WSAGetLastError ();
    //  TODO: This is not a generic way to handle this...
    if (errcode == WSAEWOULDBLOCK)
        return NULL;

    return
        (errcode == WSABASEERR) ?
            "No Error" : 
        (errcode == WSAEINTR) ?
            "Interrupted system call" : 
        (errcode == WSAEBADF) ?
            "Bad file number" : 
        (errcode == WSAEACCES) ?
            "Permission denied" : 
        (errcode == WSAEFAULT) ?
            "Bad address" : 
        (errcode == WSAEINVAL) ?
            "Invalid argument" : 
        (errcode == WSAEMFILE) ?
            "Too many open files" : 
        (errcode == WSAEWOULDBLOCK) ?
            "Operation would block" : 
        (errcode == WSAEINPROGRESS) ?
            "Operation now in progress" : 
        (errcode == WSAEALREADY) ?
            "Operation already in progress" : 
        (errcode == WSAENOTSOCK) ?
            "Socket operation on non-socket" : 
        (errcode == WSAEDESTADDRREQ) ?
            "Destination address required" : 
        (errcode == WSAEMSGSIZE) ?
            "Message too long" : 
        (errcode == WSAEPROTOTYPE) ?
            "Protocol wrong type for socket" : 
        (errcode == WSAENOPROTOOPT) ?
            "Bad protocol option" : 
        (errcode == WSAEPROTONOSUPPORT) ?
            "Protocol not supported" : 
        (errcode == WSAESOCKTNOSUPPORT) ?
            "Socket type not supported" : 
        (errcode == WSAEOPNOTSUPP) ?
            "Operation not supported on socket" : 
        (errcode == WSAEPFNOSUPPORT) ?
            "Protocol family not supported" : 
        (errcode == WSAEAFNOSUPPORT) ?
            "Address family not supported by protocol family" : 
        (errcode == WSAEADDRINUSE) ?
            "Address already in use" : 
        (errcode == WSAEADDRNOTAVAIL) ?
            "Can't assign requested address" : 
        (errcode == WSAENETDOWN) ?
            "Network is down" : 
        (errcode == WSAENETUNREACH) ?
            "Network is unreachable" : 
        (errcode == WSAENETRESET) ?
            "Net dropped connection or reset" : 
        (errcode == WSAECONNABORTED) ?
            "Software caused connection abort" : 
        (errcode == WSAECONNRESET) ?
            "Connection reset by peer" : 
        (errcode == WSAENOBUFS) ?
            "No buffer space available" : 
        (errcode == WSAEISCONN) ?
            "Socket is already connected" : 
        (errcode == WSAENOTCONN) ?
            "Socket is not connected" : 
        (errcode == WSAESHUTDOWN) ?
            "Can't send after socket shutdown" : 
        (errcode == WSAETOOMANYREFS) ?
            "Too many references can't splice" : 
        (errcode == WSAETIMEDOUT) ?
            "Connection timed out" : 
        (errcode == WSAECONNREFUSED) ?
            "Connection refused" : 
        (errcode == WSAELOOP) ?
            "Too many levels of symbolic links" : 
        (errcode == WSAENAMETOOLONG) ?
            "File name too long" : 
        (errcode == WSAEHOSTDOWN) ?
            "Host is down" : 
        (errcode == WSAEHOSTUNREACH) ?
            "No Route to Host" : 
        (errcode == WSAENOTEMPTY) ?
            "Directory not empty" : 
        (errcode == WSAEPROCLIM) ?
            "Too many processes" : 
        (errcode == WSAEUSERS) ?
            "Too many users" : 
        (errcode == WSAEDQUOT) ?
            "Disc Quota Exceeded" : 
        (errcode == WSAESTALE) ?
            "Stale NFS file handle" : 
        (errcode == WSAEREMOTE) ?
            "Too many levels of remote in path" : 
        (errcode == WSASYSNOTREADY) ?
            "Network SubSystem is unavailable" : 
        (errcode == WSAVERNOTSUPPORTED) ?
            "WINSOCK DLL Version out of range" : 
        (errcode == WSANOTINITIALISED) ?
            "Successful WSASTARTUP not yet performed" : 
        (errcode == WSAHOST_NOT_FOUND) ?
            "Host not found" : 
        (errcode == WSATRY_AGAIN) ?
            "Non-Authoritative Host not found" : 
        (errcode == WSANO_RECOVERY) ?
            "Non-Recoverable errors: FORMERR REFUSED NOTIMP" : 
        (errcode == WSANO_DATA) ?
            "Valid name no data record of requested": "error not defined"; 
}
void zmq::win_error (char *buffer_, size_t buffer_size_)
{
    DWORD errcode = GetLastError ();
    DWORD rc = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errcode, MAKELANGID(LANG_NEUTRAL,
        SUBLANG_DEFAULT), buffer_, buffer_size_, NULL );
    assert (rc);
}

#endif
