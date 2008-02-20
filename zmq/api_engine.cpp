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

#include "api_engine.hpp"

zmq::api_engine_t::api_engine_t (dispatcher_t *dispatcher_, int thread_id_) :
    ticks_max (100),
    ticks (1),
    proxy (dispatcher_, thread_id_)
{
    proxy.set_signaler (&pollset);
    thread_count = dispatcher_->get_thread_count ();
    current_thread = thread_count - 1;
}

void zmq::api_engine_t::send (int destination_thread_id_, const cmsg_t &value_)
{
    proxy.instant_write (destination_thread_id_, value_);
}

void zmq::api_engine_t::receive (cmsg_t *value_)
{
    while (true) {

        int threads_alive = proxy.get_threads_alive ();

        if (threads_alive == 1 || (threads_alive != thread_count &&
              ticks == ticks_max)) {

            uint32_t signals;
            signals = threads_alive != 1 ? pollset.check () : pollset.poll ();
            
            for (int thread_nbr = 0; thread_nbr != thread_count;
                  thread_nbr ++) {
                if (signals & 0x0001)
                    proxy.revive (thread_nbr);
                signals >>= 1;
            }
        }

        current_thread = (current_thread + 1) % thread_count;
        if (proxy.read (current_thread, value_)) {
            ticks --;
            if (!ticks)
                ticks = ticks_max;
            return;
        }
    }
}

