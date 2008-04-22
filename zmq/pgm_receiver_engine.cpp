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

#include <poll.h>

#include "pgm_receiver_engine.hpp"

zmq::pgm_receiver_engine_t::pgm_receiver_engine_t (dispatcher_t *dispatcher_, int engine_id_,
      const char *network_, uint16_t port_, int destination_engine_id_):
    proxy (dispatcher_, engine_id_),
    decoder (&proxy, destination_engine_id_),
    epgm_socket (true, false, network_, port_)
{

}

zmq::pgm_receiver_engine_t::~pgm_receiver_engine_t ()
{

}

void zmq::pgm_receiver_engine_t::set_signaler (i_signaler *signaler_)
{
    proxy.set_signaler (signaler_);
}

int zmq::pgm_receiver_engine_t::get_fd_count ()
{
    int nfds = epgm_socket.get_fd_count (EPOLLIN);
    assert (nfds == pgm_receiver_fds);

    return nfds;
}

int zmq::pgm_receiver_engine_t::get_pfds (pollfd *pfd_, int count_)
{
    return epgm_socket.get_pfds (pfd_, count_, EPOLLIN);
}

void zmq::pgm_receiver_engine_t::revive (pollfd *pfd_, int count_, int engine_id_)
{
    assert (0);
}

void zmq::pgm_receiver_engine_t::in_event (pollfd *pfd_, int count_, int index_)
{
    assert (count_ == pgm_receiver_fds);

    switch (index_) {
        case pgm_recv_fd_idx:
            // POLLIN event from recv socket
            {
                iovec iov;
    
                size_t nbytes = epgm_socket.read_one_pkt_with_offset (&iov);

                printf ("received %iB, %s(%i)\n", (int)nbytes, __FILE__, __LINE__);

                // No data received
                if (!nbytes) {
                    return;
                }

                //  Push the data to the decoder
                printf ("writting %iB into decoder, %s(%i)\n", (int)iov.iov_len, 
                    __FILE__, __LINE__);
                decoder.write ((unsigned char*)iov.iov_base, iov.iov_len);

                //  Flush any messages decoder may have produced to the dispatcher
                proxy.flush ();
            }
            break;
        default:
            assert (0);
    }
}

void zmq::pgm_receiver_engine_t::out_event (pollfd *pfd_, int count_, int index_)
{
    assert (0);
}
