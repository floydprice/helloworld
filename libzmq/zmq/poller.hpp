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

#ifndef __ZMQ_POLLER_HPP_INCLUDED__
#define __ZMQ_POLLER_HPP_INCLUDED__

#include <vector>
#include <cstdlib>

#include <zmq/i_poller.hpp>
#include <zmq/i_pollable.hpp>
#include <zmq/dispatcher.hpp>
#include <zmq/ysocketpair.hpp>
#include <zmq/thread.hpp>
#include <zmq/fd.hpp>

namespace zmq
{

    enum event_t
    {
        //  The file contains some data.
        event_in,

        //  The file can accept some more data.
        event_out,

        //  There was an error on file descriptor.
        event_err
    };

    template <class T> class poller_t : public i_poller
    {
    public:

        static i_thread *create (dispatcher_t *dispatcher_);
        
        //  i_poller implementation.
        int get_thread_id ();
        void send_command (i_thread *destination_, const command_t &command_);
        void stop ();
        void destroy ();
        handle_t add_fd (fd_t fd_, i_pollable *engine_);
        void rm_fd (handle_t handle_);
        void set_pollin (handle_t handle_);
        void reset_pollin (handle_t handle_);
        void set_pollout (handle_t handle_);
        void reset_pollout (handle_t handle_);

        //  Callback function called by event_monitor.
        bool process_event (i_pollable *engine_, event_t event_);

    private:

        poller_t (dispatcher_t *dispatcher_);
        ~poller_t ();

        //  Main worker thread routine.
        static void worker_routine (void *arg_);

        //  Main routine (non-static) - called from worker_routine.
        void loop ();

        //  Processes individual command. Returns false if the thread should
        //  terminate.
        bool process_command (const command_t &command_);

        //  Pointer to dispatcher.
        dispatcher_t *dispatcher;

        //  Thread ID allocated for the poll thread by dispatcher.
        int thread_id;

        //  Poll thread gets notifications about incoming commands using
        //  this socketpair.
        ysocketpair_t signaler;

        //  Handle associated with signaler's file descriptor.
        handle_t signaler_handle;

        //  Handle of the physical thread doing the I/O work.
        thread_t worker;

        //  We perform I/O multiplexing using event monitor.
        T event_monitor;
    };

}

template <class T>
zmq::i_thread *zmq::poller_t <T>::create (dispatcher_t *dispatcher_)
{
    //  Create the object.
    poller_t <T> *poller = new poller_t <T> (dispatcher_);
    assert (poller);

    //  Start the thread.
    poller->worker.start (worker_routine, poller);

    return poller;
}

template <class T>
void zmq::poller_t <T>::destroy ()
{
    //  Stop the thread.
    worker.stop ();

    //  TODO: At this point terminal handshaking should be done.
    //  Afterwards 'delete this' can be executed. 
}

template <class T>
zmq::poller_t <T>::poller_t (dispatcher_t *dispatcher_) :
    dispatcher (dispatcher_)
{
    signaler_handle = event_monitor.add_fd (signaler.get_fd (), NULL);
    event_monitor.set_pollin (signaler_handle);

    //  Register the thread with command dispatcher.
    thread_id = dispatcher->allocate_thread_id (this, &signaler);
}

template <class T>
zmq::poller_t <T>::~poller_t ()
{
    event_monitor.rm_fd (signaler_handle);
}

template <class T>
int zmq::poller_t <T>::get_thread_id ()
{
    return thread_id;
}

template <class T>
void zmq::poller_t <T>::send_command (i_thread *destination_,
    const command_t &command_)
{
    if (destination_ == (i_thread*) this)
        process_command (command_);
    else
        dispatcher->write (thread_id,
            destination_->get_thread_id (), command_);
}

template <class T>
void zmq::poller_t <T>::stop ()
{
    //  'to-self' command pipe is used solely for the 'stop' command.
    //  This way there's no danger of 2 threads accessing the pipe
    //  at the same time.
    command_t cmd;
    cmd.init_stop ();
    dispatcher->write (thread_id, thread_id, cmd);
}

template <class T>
zmq::handle_t zmq::poller_t <T>::add_fd (fd_t fd_, i_pollable *engine_)
{
    return event_monitor.add_fd (fd_, engine_);
}

template <class T>
void zmq::poller_t <T>::rm_fd (handle_t handle_)
{
    event_monitor.rm_fd (handle_);
}

template <class T>
void zmq::poller_t <T>::set_pollin (handle_t handle_)
{
    event_monitor.set_pollin (handle_);
}

template <class T>
void zmq::poller_t <T>::reset_pollin (handle_t handle_)
{
    event_monitor.reset_pollin (handle_);
}

template <class T>
void zmq::poller_t <T>::set_pollout (handle_t handle_)
{
    event_monitor.set_pollout (handle_);
}

template <class T>
void zmq::poller_t <T>::reset_pollout (handle_t handle_)
{
    event_monitor.reset_pollout (handle_);
}

template <class T>
void zmq::poller_t <T>::worker_routine (void *arg_)
{
    poller_t <T> *self = (poller_t <T>*) arg_;
    self->loop ();
}

template <class T>
void zmq::poller_t <T>::loop ()
{
    while (true) {
        if (event_monitor.process_events (this))
           break;
    }
}

template <class T>
bool zmq::poller_t <T>::process_event (i_pollable *engine_, event_t event_)
{
    if (!engine_) {
        uint32_t signals = signaler.check ();
        assert (signals);

        //  Iterate through all the threads in the process and find out
        //  which of them sent us commands.
        for (int source_thread_id = 0;
              source_thread_id != dispatcher->get_thread_count ();
              source_thread_id ++) {
            if (signals & (1 << source_thread_id)) {

                //  Read all the commands from particular thread.
                command_t command;
                while (dispatcher->read (source_thread_id, thread_id,
                      &command))
                    if (!process_command (command))
                        return true;
            }
        }
    }
    else {
        switch (event_) {
        case event_out:
            engine_->out_event ();
            break;
        case event_in:
        case event_err:
            engine_->in_event ();
            break;
        }
    }
    return false;
}

template <class T>
bool zmq::poller_t <T>::process_command (const command_t &command_)
{
    i_engine *engine;

    switch (command_.type) {

    //  Exit the working thread.
    case command_t::stop:
        break;

    //  Register the engine supplied with the poll thread.
    case command_t::register_engine:

        //  Ask engine to register itself.
        engine = command_.args.register_engine.engine;
        engine->cast_to_pollable ()->register_event (this);
        break;

    //  Unregister the engine.
    case command_t::unregister_engine:

        //  Ask engine to unregister itself.
        engine = command_.args.unregister_engine.engine;
        engine->cast_to_pollable ()->unregister_event ();
        break;

    //  Forward the command to the specified engine.
    case command_t::engine_command:
        {
            //  Forward the command to the engine.
            engine = command_.args.engine_command.engine;
            const engine_command_t &engcmd =
                command_.args.engine_command.command;
            switch (engcmd.type) {
            case engine_command_t::revive:
                engine->revive (engcmd.args.revive.pipe);
                break;
            case engine_command_t::head:
                engine->head (engcmd.args.head.pipe, engcmd.args.head.position);
                break;
            case engine_command_t::send_to:
                engine->send_to (engcmd.args.send_to.pipe);
                break;
            case engine_command_t::receive_from:
                engine->receive_from (engcmd.args.receive_from.pipe);
                break;
            case engine_command_t::terminate_pipe:
                engine->terminate_pipe (engcmd.args.terminate_pipe.pipe);
                break;
            case engine_command_t::terminate_pipe_ack:
                engine->terminate_pipe_ack (
                    engcmd.args.terminate_pipe_ack.pipe);
                break;
            default:

                //  Unknown engine command.
                assert (false);
            }
            break;
        }

    //  Unknown command.
    default:
        assert (false);
    }

    if (command_.type == command_t::stop)
        return false;
    return true;
}

#endif
