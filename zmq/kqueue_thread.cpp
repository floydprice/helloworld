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

#include "platform.hpp"

#if defined (ZMQ_HAVE_FREEBSD) || defined (ZMQ_HAVE_OPENBSD) ||\
    defined (ZMQ_HAVE_OSX)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "err.hpp"
#include "kqueue_thread.hpp"

using namespace zmq;

i_thread *kqueue_thread_t::create (dispatcher_t *dispatcher_)
{
    return new kqueue_thread_t (dispatcher_);
}

kqueue_thread_t::kqueue_thread_t (dispatcher_t *dispatcher_) :
    dispatcher (dispatcher_)
{
    //  Create event queue
    kqueue_fd = kqueue ();
    errno_assert (kqueue_fd != -1);

    //  Add signaller into our interest set.
    kevent_add (signaler.get_fd (), EVFILT_READ, NULL);

    //  Register the thread with command dispatcher.
    thread_id = dispatcher->allocate_thread_id (&signaler);

    //  Create the worker thread.
    worker = new thread_t (worker_routine, this);
}

kqueue_thread_t::~kqueue_thread_t ()
{
    //  Send a 'stop' event ot the worker thread.
    //  TODO: Analyse whether using the 'to-self' command pipe here
    //        is appropriate.
    command_t cmd;
    cmd.init_stop ();
    dispatcher->write (thread_id, thread_id, cmd);

    //  Wait till the worker thread terminates.
    delete worker;

    dispatcher->deallocate_thread_id (thread_id);
    close (kqueue_fd);
}

int kqueue_thread_t::get_thread_id ()
{
    return thread_id;
}

void kqueue_thread_t::send_command (i_thread *destination_,
    const command_t &command_)
{
    dispatcher->write (thread_id, destination_->get_thread_id (), command_);
}

kqueue_thread_t::poll_entry *
kqueue_thread_t::new_poll_entry (int fd_, i_pollable *engine_)
{
    struct poll_entry *pe;

    pe = (struct poll_entry*) malloc (sizeof (struct poll_entry));
    if (pe) {
        pe->fd = fd_;
        pe->flag_pollin = 0;
        pe->flag_pollout = 0;
        pe->engine = engine_;
    }

    return pe;
}

void kqueue_thread_t::kevent_add (int fd_, short filter_, void *udata_)
{
    struct kevent ev;

    EV_SET (&ev, fd_, filter_, EV_ADD, 0, 0, udata_);
    int rc = kevent (kqueue_fd, &ev, 1, NULL, 0, NULL);
    errno_assert (rc != -1);
}

void kqueue_thread_t::kevent_delete (int fd_, short filter_)
{
    struct kevent ev;

    EV_SET (&ev, fd_, filter_, EV_DELETE, 0, 0, NULL);
    int rc = kevent (kqueue_fd, &ev, 1, NULL, 0, NULL);
    errno_assert (rc != -1);
}

handle_t kqueue_thread_t::add_fd (int fd_, i_pollable *engine_)
{
    struct poll_entry *pe = new_poll_entry (fd_, engine_);
    assert (pe != NULL);

    object_table_t::iterator it = engines.find (engine_);
    if (it == engines.end ())
        engines.insert (object_table_t::value_type (engine_, 1));
    else
        it->second++;

    handle_t handle;
    handle.ptr = pe;
    return handle;
}

void kqueue_thread_t::rm_fd (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;

    object_table_t::iterator it = engines.find (pe->engine);
    assert (it != engines.end ());

    if (--it->second == 0)
        engines.erase (it);

    if (pe->flag_pollin)
        kevent_delete (pe->fd, EVFILT_READ);
    if (pe->flag_pollout)
        kevent_delete (pe->fd, EVFILT_WRITE);

    free (pe);
}

void kqueue_thread_t::set_pollin (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;
    pe->flag_pollin = true;
    kevent_add (pe->fd, EVFILT_READ, pe);
}

void kqueue_thread_t::reset_pollin (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;
    pe->flag_pollin = false;
    kevent_delete (pe->fd, EVFILT_READ);
}

void kqueue_thread_t::speculative_read (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;
    pe->flag_pollin = true;
    kevent_add (pe->fd, EVFILT_READ, pe);
    pe->engine->in_event ();
}

void kqueue_thread_t::set_pollout (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;
    pe->flag_pollout = true;
    kevent_add (pe->fd, EVFILT_WRITE, pe);
}

void kqueue_thread_t::reset_pollout (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;
    pe->flag_pollout = false;
    kevent_delete (pe->fd, EVFILT_WRITE);
}

void kqueue_thread_t::speculative_write (handle_t handle_)
{
    struct poll_entry *pe = (struct poll_entry*) handle_.ptr;
    pe->flag_pollout = true;
    kevent_add (pe->fd, EVFILT_WRITE, pe);
    pe->engine->out_event ();
}

void kqueue_thread_t::worker_routine (void *arg_)
{
    kqueue_thread_t *self = (kqueue_thread_t*) arg_;
    self->loop ();
}

void kqueue_thread_t::loop ()
{
    while (true) {
        struct kevent ev_buf[epoll_max_events];

        //  Wait for events.
        int n = kevent (kqueue_fd, NULL, 0,
                        &ev_buf[0], epoll_max_events, NULL);
        errno_assert (n != -1);

        for (struct kevent *ev = &ev_buf[0]; ev < &ev_buf[n]; ev++) {
            if (ev->ident == (u_int) signaler.get_fd ()) {
                assert ((ev->flags & EV_EOF) == 0);
                uint32_t signals = signaler.check ();
                assert (signals);
                if (!process_commands (signals))
                    return;
            }
            else {
                struct poll_entry *pe = (struct poll_entry*) ev->udata;
                assert (pe != NULL);

                //  Handle errors
                if (ev->flags & EV_EOF)
                    pe->engine->error_event ();

                //  Process out events from the engine.
                if (ev->filter == EVFILT_WRITE)
                    pe->engine->out_event ();

                //  Process in events from the engine.
                if (ev->filter == EVFILT_READ)
                    pe->engine->in_event ();
            }
        }
    }
}

bool kqueue_thread_t::process_commands (uint32_t signals_)
{
    i_engine *engine;

    //  Iterate through all the threads in the process and find out which
    //  of them sent us commands.
    for (int i = 0; i != dispatcher->get_thread_count (); i++) {
        if (signals_ & 1 << i) {

            //  Read all the commands from particular thread.
            command_t command;
            while (dispatcher->read (i, thread_id, &command)) {

                switch (command.type) {

                //  Exit the working thread.
                case command_t::stop:
                    return false;

                //  Register the engine supplied with the poll thread.
                case command_t::register_engine:

                    //  Ask engine to register itself.
                    engine = command.args.register_engine.engine;
                    assert (engine->type () == engine_type_fd);
                    ((i_pollable*) engine)->register_event (this);
                    break;

                //  Unregister the engine.
                case command_t::unregister_engine:

                    //  Assert that engine still exists.
                    //  TODO: We should somehow make sure this won't happen.
                    engine = command.args.unregister_engine.engine;
                    assert (engines.find (engine) != engines.end ());

                    //  Ask engine to unregister itself.
                    assert (engine->type () == engine_type_fd);
                    ((i_pollable*) engine)->unregister_event ();
                    break;

                //  Forward the command to the specified engine.
                case command_t::engine_command:

                    //  Check whether engine still exists.
                    //  TODO: We should somehow make sure this won't happen.

                    //  Forward the command to the engine.
                    //  TODO: If the engine doesn't exist drop the command.
                    //        However, imagine there's another engine
                    //        incidentally allocated on the same address.

                    engine = command.args.register_engine.engine;
                    if (engines.find (engine) != engines.end ())
                        engine->process_command (
                            command.args.engine_command.command);
                    break;

                //  Unknown command.
                default:
                    assert (false);
                }
            }
        }
    }
    return true;
}

#endif
