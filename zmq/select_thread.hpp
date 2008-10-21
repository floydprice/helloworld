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

#ifndef __ZMQ_SELECT_THREAD_HPP_INCLUDED__
#define __ZMQ_SELECT_THREAD_HPP_INCLUDED__

#include "i_thread.hpp"
#include "i_pollable.hpp"
#include "i_poller.hpp"
#include "dispatcher.hpp"
#include "ysocketpair.hpp"
#include "thread.hpp"
#include "platform.hpp"
#include "declspec_export.hpp"

#ifndef ZMQ_HAVE_WINDOWS
#include <poll.h>
#endif

#if defined ZMQ_HAVE_LINUX || defined ZMQ_HAVE_FREEBSD || defined ZMQ_HAVE_OSX || defined ZMQ_HAVE_WINDOWS

#include <stddef.h>
#include <assert.h>

namespace zmq
{

    //  Poll thread is a I/O thread that waits for events from engines
    //  using POSIX poll function and schedules handling of the signals
    //  by individual engines. Engine compatible with poll thread should
    //  expose i_pollable interface.

    class select_thread_t : public i_thread, public i_poller
    {
    public:

        //  Create a poll thread.
        declspec_export static i_thread *create (dispatcher_t *dispatcher_);

        //  Destroy the poll thread.
        declspec_export ~select_thread_t ();

        //  i_thread implementation.
        declspec_export int get_thread_id ();
        declspec_export void send_command (i_thread *destination_, const command_t &command_);

        //  i_poller implementation.
        declspec_export int add_fd (int fd_, i_pollable *engine_);
        declspec_export void rm_fd (int handle_);
        declspec_export void set_pollin (int handle_);
        declspec_export void reset_pollin (int handle_);
        declspec_export void speculative_read (int handle_);
        declspec_export void set_pollout (int handle_);
        declspec_export void reset_pollout (int handle_);
        declspec_export void speculative_write (int handle_);

    private:

        select_thread_t (dispatcher_t *dispatcher_);

        //  Main worker thread routine.
        static void worker_routine (void *arg_);

        //  Main routine (non-static) - called from worker_routine.
        void loop ();

        //  Processes commands from other threads. Returns false if the thread
        //  should terminate.
        bool process_commands (uint32_t signals_);

        //  Pointer to dispatcher.
        dispatcher_t *dispatcher;

        //  Thread ID allocated for the poll thread by dispatcher.
        int thread_id;

        //  Poll thread gets notifications about incoming commands using
        //  this socketpair.
        ysocketpair_t signaler;

        //  Handle of the physical thread doing the I/O work.
        thread_t *worker;

        //  Set of file descriptors that are used to retreive
        //  information for fd_set.
        typedef std::vector <int> fd_set_t;
        fd_set_t fdset;
        
        fd_set source_set_in;
        fd_set source_set_out;
        fd_set result_set_in;
        fd_set result_set_out;
        fd_set error_set;
        
        // Maximum file descriptor plus 1.
        int maxfdp1;
        
        

        //  List of engines handled by this poll thread.
        typedef std::vector <i_pollable*> engines_t;
        engines_t engines;

        select_thread_t (const select_thread_t&);
        void operator = (const select_thread_t&);
    };

}

#endif

#endif
