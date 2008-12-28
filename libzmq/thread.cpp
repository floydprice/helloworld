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

#include <zmq/thread.hpp>
#include <zmq/err.hpp>
#include <zmq/platform.hpp>

#ifdef ZMQ_HAVE_WINDOWS

void zmq::thread_t::start (thread_fn *tfn_, void *arg_)
{
    tfn = tfn_;
    arg =arg_;
    descriptor = (HANDLE) _beginthreadex (NULL, 0,
        &zmq::thread_t::thread_routine, this, 0 , NULL);
    win_assert (descriptor != NULL);    
}

void zmq::thread_t::stop ()
{
    DWORD rc = WaitForSingleObject (descriptor, INFINITE);
    win_assert (rc != WAIT_FAILED);
}

unsigned int __stdcall zmq::thread_t::thread_routine (void *arg_)
{
    thread_t *self = (thread_t*) arg_;
    self->tfn (self->arg);
    return 0;
}

#else

void zmq::thread_t::start (thread_fn *tfn_, void *arg_)
{
    tfn = tfn_;
    arg =arg_;
    int rc = pthread_create (&descriptor, NULL, thread_routine, this);
    errno_assert (rc == 0);
}

void zmq::thread_t::stop ()
{
    int rc = pthread_join (descriptor, NULL);
    errno_assert (rc == 0);
}

void *zmq::thread_t::thread_routine (void *arg_)
{
    thread_t *self = (thread_t*) arg_;   
    self->tfn (self->arg);
    return NULL;
}

#endif





