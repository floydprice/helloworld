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

#ifndef __ZMQ_ERR_HPP_INCLUDED__
#define __ZMQ_ERR_HPP_INCLUDED__

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <zmq/export.hpp>
#include <zmq/platform.hpp>

#ifdef ZMQ_HAVE_WINDOWS
#include <winsock2.h>
#else
#include <netdb.h>
#endif

#ifdef ZMQ_HAVE_WINDOWS

namespace zmq
{

    ZMQ_EXPORT const char *wsa_error ();
    ZMQ_EXPORT void win_error (char *buffer_, size_t buffer_size_);
  
}

//  Provides convenient way to check WSA-style errors on Windows.
#define wsa_assert(x) do { if (!(x)){\
    const char *errstr = zmq::wsa_error ();\
    if (errstr != NULL) {\
    fprintf (stderr, "Assertion failed: %s (%s:%d)\n", errstr, \
        __FILE__, __LINE__);\
        abort ();\
    }\
}} while (false)

//  Provides convenient way to check GetLastError-style errors on Windows.
#define win_assert(x) do { if (!(x)) {\
    char errstr [256];\
    zmq::win_error (errstr, 256);\
    fprintf (stderr, "Assertion failed: %s (%s:%d)\n", errstr, \
        __FILE__, __LINE__);\
    abort ();\
}} while (false)

#endif

//  Provides convenient way to check for errno-style errors.
#define errno_assert(x) do { if (!(x)) {\
    perror (NULL);\
    fprintf (stderr, "%s (%s:%d)\n", #x, __FILE__, __LINE__);\
    abort ();\
}} while (false)

//  Provides convenient way to check for POSIX errors.
#define posix_assert(x) do {\
   fprintf (stderr, "%s (%s:%d)\n", strerror (x), __FILE__, __LINE__);\
   abort ();\
} while (false)

//  Provides convenient way to check for errors from getaddrinfo.
#define gai_assert(x) do { if (x) {\
    const char *errstr = gai_strerror (x);\
    fprintf (stderr, "%s (%s:%d)\n", errstr, __FILE__, __LINE__);\
    abort ();\
}} while (false)

#endif
