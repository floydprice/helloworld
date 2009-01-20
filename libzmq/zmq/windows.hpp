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

#ifndef __ZMQ_WINDOWS_HPP_INCLUDED__
#define __ZMQ_WINDOWS_HPP_INCLUDED__

// The purpose of this header file is to turn on only the items zmq needs
// on the windows platform 

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOUSER  // No USER defines and routines
#define NOUSER
#endif
#ifndef NOMCX   // No Modem Configuration Extensions
#define NOMCX
#endif
#ifndef NOIME   // No Input Method Editor
#define NOIME
#endif
#ifndef NOSOUND // No Sound driver routines
#define NOSOUND
#endif

#include <windows.h>

// enable winsock (not included when WIN32_LEAN_AND_MEAN is defined)
#if(_WIN32_WINNT >= 0x0400)
#include <winsock2.h>
#include <mswsock.h>
#else
#include <winsock.h>
#endif

#endif // __ZMQ_WINDOWS_HPP_INCLUDED__
