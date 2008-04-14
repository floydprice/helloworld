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

#include "poll_thread.hpp"
#include "err.hpp"

zmq::poll_thread_t::poll_thread_t (i_pollable *engine_) :
    engine (engine_)
{
    //  Attach the engine to the dispatcher
    engine->set_signaler (&signaler);

    //  Create the worker thread
    int rc = pthread_create (&worker, NULL, worker_routine, this);
    errno_assert (rc == 0);
}

zmq::poll_thread_t::~poll_thread_t ()
{
    //  Ask worker thread to stop
    signaler.signal (stop_event);

    //  Wait till worker thread terminates
    int rc = pthread_join (worker, NULL);
    errno_assert (rc == 0);
}

void *zmq::poll_thread_t::worker_routine (void *arg_)
{
    poll_thread_t *self = (poll_thread_t*) arg_;
    self->loop ();
    return 0;
}

void zmq::poll_thread_t::loop ()
{
    bool stop = false;

//    pollfd pfd [4];
//    int fds [3];
    int nfds = engine->get_fd_count ();

    // Need to add one pollfd structure for administrative commands
    nfds++;

    pollfd pfd; 

    //  Poll for administrative commands (revive & stop commands)
    pfd [0].fd = signaler.get_fd ();
    pfd [0].events = POLLIN;

    //  Poll for events from the engine
    nfds = engine->get_fd (fds, 3);
    for (int i = 0; i < nfds; i++) {
        pfd [1 + i].fd = fds [i];
    }

    while (true)
    {
        //  Adjust the events to wait - the engine chooses the events
        pfd [1].events = engine->get_events ();
//        pfd [2].events = POLLIN;

/*        if (nfds == 3)
            pfd [3].events = engine->get_events ();
*/
        //  Wait for events
        int rc = poll (pfd, 1 + nfds, -1);
        errno_assert (rc != -1);
        assert (!(pfd [0].revents & (POLLERR | POLLHUP | POLLNVAL)));
        assert (!(pfd [1].revents & (POLLERR | POLLNVAL)));

        if (pfd [0].revents & POLLIN) {

            //  Process for administrative commands. Commands are read in
            //  batches to speed the processing up.
            unsigned char events [256];
            ssize_t nbytes = recv (pfd [0].fd, events, 256,
                MSG_DONTWAIT);
            errno_assert (nbytes != -1);
            for (int event = 0; event != nbytes; event ++) {

                if (events [event] == stop_event) {

                    //  Stop command :
                    //  If there are no messages to send, quit immediately
                    //  Otherwise wait till all the messages are sent
                    if (!(engine->get_events () & POLLOUT))
                        return;
                    else
                        stop = true;
                }
                else
                    //  Revive command
                    engine->revive (events [event]);
            }
        }

        if (pfd [1].revents & POLLOUT) {

            //  Process out event from the engine
            engine->out_event ();
            if (stop && !(engine->get_events () & POLLOUT))
                return;
        }

        if (pfd [1].revents & (POLLIN | POLLHUP)) {
            printf ("POLLIN from network, %s(%i)\n", __FILE__, __LINE__);
            //  Process in event from the engine
            engine->in_event ();
        }

        if (nfds == 2 && (pfd [2].revents & (POLLIN | POLLHUP))) {
            printf ("POLLIN from network, %s(%i)\n", __FILE__, __LINE__);
            assert (0);
        }

        if (nfds == 3 && (pfd [3].revents & POLLOUT)) {
            printf ("POLLOUT from network, %s(%i)\n", __FILE__, __LINE__);
            engine->out_event ();

            if (stop && !(engine->get_events () & POLLOUT))
                return;

        }


    }
}
