/*
    Copyright (c) 2007-2008 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_DISPATCHER_HPP_INCLUDED__
#define __ZMQ_DISPATCHER_HPP_INCLUDED__

#include <vector>
#include <pthread.h>

#include "i_context.hpp"
#include "i_signaler.hpp"
#include "ypipe.hpp"
#include "locator.hpp"
#include "config.hpp"

namespace zmq
{

    //  Dispatcher implements bidirectional thread-safe ultra-efficient
    //  passing of commands between N threads.
    //
    //  It's designed to encapsulate all the cross-cutting functionality
    //  between two different threads. Namely, it consists of a pipe to pass
    //  commands and signaler to wake up receiver thread when new commands
    //  are available.
    //
    //  Note that dispatcher is inefficient for passing messages within a thread
    //  (sender thread = receiver thread) as it performs unneccessary atomic
    //  operations. However, given that within-thread message transfer
    //  is not a cross-cutting functionality, the optimisation is not part
    //  of the class and should be implemented by individual threads.

    class dispatcher_t : public i_context
    {
    public:

        //  Create the dispatcher object. The actual number of threads
        //  supported will be thread_count_ + 1 (standard worker threads +
        //  one administrative thread). The administrative thread is
        //  specific in that it is synchronised and can be used from any
        //  thread whatsoever.
        //  'address' and 'port' paramters specify the global locator to use.
        dispatcher_t (int thread_count_, const char *address_, uint16_t port_);

        //  Destroy the dispatcher object
        ~dispatcher_t ();

        //  Returns number of threads dispatcher is preconfigured for
        inline int get_thread_count ()
        {
            return thread_count;
        }

        //  Write messages to dispatcher. If more commands are to be written,
        //  they are supplied in a linked list. Individual items in the list
        //  should be allocated using new operator.
        inline void write (int source_thread_id_, int destination_thread_id_,
            const command_t &value_)
        {
            command_pipe_t &pipe = pipes [source_thread_id_ *
                  thread_count + destination_thread_id_];
            pipe.write (value_);
            if (!pipe.flush ())
                signalers [destination_thread_id_]->signal (source_thread_id_);
        }

        //  Read message from the dispatcher. Returns false if there is no
        //  command available.
        inline bool read (int source_thread_id_, int destination_thread_id_,
            command_t *command_)
        {
            return pipes [source_thread_id_ * thread_count +
                destination_thread_id_].read (command_);
        }

        //  Assign an thread ID to the caller
        //  Regiter the supplied signaler with the thread
        int allocate_thread_id (i_signaler *signaler_);

        //  Return thread ID to the pool of free thread IDs
        void deallocate_thread_id (int thread_id_);

        //  i_context (administrative context) implementation
        int get_thread_id ();
        void send_command (i_context *destination_, const command_t &command_);

        //  Get locator reference
        //  TODO: Is this the right place to place locator?
        inline class locator_t &get_locator ()
        {
            return locator;
        }

    private:

        typedef ypipe_t <command_t, true,
            command_pipe_granularity> command_pipe_t;

        enum {admin_thread_id = 0};

        int thread_count;
        command_pipe_t *pipes;
        std::vector <i_signaler*> signalers;

        //  Vector specifying which thread IDs are used and which are not.
        //  The access to the vector is synchronised using mutex - this is OK
        //  as the performance of thread ID assignment is not critical for
        //  the performance of the system as a whole. The mutex is also used
        //  to sync the commands from the administrative context.
        std::vector <bool> used;
        pthread_mutex_t mutex;

        //  TODO: Locator is at the moment embedded in the dispatcher -
        //  We should find a better place for it
        locator_t locator;
    };

    //  Hack to report connection breakages to the client
    typedef bool (error_handler_t) ();

    inline error_handler_t *&get_error_handler ()
    {
        static error_handler_t *eh = NULL;
        return eh;
    }

    inline void set_error_handler (error_handler_t *eh_)
    {
        get_error_handler () = eh_;
    }

}

#endif

