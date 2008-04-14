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

#include "pgm_sender_engine.hpp"

zmq::pgm_sender_engine_t::pgm_sender_engine_t (dispatcher_t *dispatcher_, int engine_id_,
      const char *network_, uint16_t port_,
      int source_engine_id_/*, int destination_engine_id_,*/
      /*size_t writebuf_size_, size_t readbuf_size_*/) :
    proxy (dispatcher_, engine_id_),
    encoder (&proxy, source_engine_id_),
    pgm_sender (network_, port_),
    events (POLLOUT),
    writebuf_size (8192),
    write_size (0),
    write_pos (0)

{
    writebuf = new unsigned char [writebuf_size];
    assert (writebuf);
}

zmq::pgm_sender_engine_t::~pgm_sender_engine_t ()
{
    delete [] writebuf;
}

void zmq::pgm_sender_engine_t::set_signaler (i_signaler *signaler_)
{
    proxy.set_signaler (signaler_);
}

int zmq::pgm_sender_engine_t::get_fd (int *fds_, int nfds_)
{
    return pgm_sender.get_fd (fds_, nfds_);
}

short zmq::pgm_sender_engine_t::get_events ()
{
    return events;
}

void zmq::pgm_sender_engine_t::revive (int engine_id_)
{
    //  There is at least one engine that has messages ready - start polling
    //  the socket for writing.
    proxy.revive (engine_id_);
    events |= POLLOUT;
}

void zmq::pgm_sender_engine_t::in_event ()
{
    assert (0);
/*
    iovec *iovs;
    
    size_t nbytes = pgm_receiver.read_msg (&iovs);

    printf ("received %iB, %s(%i)\n", (int)nbytes, __FILE__, __LINE__);

    if (!nbytes) {
        return;
    }

    //  Push the data to the decoder
    while (nbytes > 0) {
        printf ("writting %iB into decoder, %s(%i)\n", (int)iovs->iov_len, 
            __FILE__, __LINE__);
        decoder.write ((unsigned char*)iovs->iov_base, iovs->iov_len);
        nbytes -= iovs->iov_len;
        iovs++;
    }

    //  Flush any messages decoder may have produced to the dispatcher
    proxy.flush ();
*/
}

void zmq::pgm_sender_engine_t::out_event (int fd)
{
    //  If write buffer is empty, try to read new data from the encoder
    if (write_pos == write_size) {

        write_size = encoder.read (writebuf, writebuf_size);
        write_pos = 0;

        printf ("read %iB from encoder, %s(%i)\n", (int)write_size, __FILE__, __LINE__);

        //  If there are no data to write stop polling for output
        if (!write_size) {
            events ^= POLLOUT;
            printf ("POLLOUT stopped, %s(%i)\n", __FILE__, __LINE__);
        }
    }

    //  If there are any data to write in write buffer, write as much as
    //  possible to the socket.
    if (write_pos < write_size) {
        size_t nbytes = pgm_sender.write_pkt (writebuf + write_pos,
            write_size - write_pos, 0);

        printf ("wrote %iB/%iB, %s(%i)\n", write_size - write_pos, nbytes, __FILE__, __LINE__);
        assert (write_size - write_pos >= nbytes);

        write_pos += nbytes;
    }

}
