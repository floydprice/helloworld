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

//  Test whether a message batch can be passed between two threads

#include <assert.h>
#include <pthread.h>

#include "../zmq/dispatcher.hpp"
#include "../zmq/api_engine.hpp"
using namespace zmq;

void *worker_routine (void *arg)
{
    api_engine_t *engine0 = (api_engine_t*) arg;

    cmsg_t msg = {(void*) "ABCDEF", 6, NULL};
    for (int msg_nbr = 0; msg_nbr != 10000; msg_nbr ++)
        engine0->send (1, msg);

    return NULL;
}

int main ()
{
    //  Create a dispatcher with two API engines plugged-in
    dispatcher_t dispatcher (2);
    api_engine_t engine0 (&dispatcher);
    api_engine_t engine1 (&dispatcher);

    //  Start the sender thread
    pthread_t worker;
    int rc = pthread_create (&worker, NULL, worker_routine, (void*) &engine0);
    assert (rc == 0);

    for (int msg_nbr = 0; msg_nbr != 10000; msg_nbr++) {

        //  Receive the message
        cmsg_t msg;
        init_cmsg (msg);
        engine1.receive (&msg);
        assert (msg.size == 6);
        free_cmsg (msg);
    }

    //  Wait till worker thread terminates
    rc = pthread_join (worker, NULL);
    assert (rc == 0);

    return 0;
}
