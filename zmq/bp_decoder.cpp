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

#include "bp_decoder.hpp"
#include "wire.hpp"

zmq::bp_decoder_t::bp_decoder_t (demux_t *demux_) :
    demux (demux_)
{
    next_step (tmpbuf, 1, &bp_decoder_t::one_byte_size_ready);
}

void zmq::bp_decoder_t::one_byte_size_ready ()
{
    //  First byte of size is read. If it is 0xff read 8-byte size.
    //  Otherwise allocate the buffer for message data and
    //  read the data into it.
    if (*tmpbuf == 0xff)
        next_step (tmpbuf, 8, &bp_decoder_t::eight_byte_size_ready);
    else {
        msg.size = *tmpbuf;
        msg.data = malloc (*tmpbuf);
        assert (msg.data);
        msg.ffn = free;

        next_step (msg.data, *tmpbuf, &bp_decoder_t::message_ready);
    }
}

void zmq::bp_decoder_t::eight_byte_size_ready ()
{
    //  8-byte size is read. Allocate the buffer for message body and
    //  read data into it.
    msg.size = get_uint64 (tmpbuf);
    msg.data = malloc (msg.size);
    assert (msg.data);
    msg.ffn = free;

    next_step (msg.data, msg.size, &bp_decoder_t::message_ready);
}

void zmq::bp_decoder_t::message_ready ()
{
    //  Message is completely read. Push it to the dispatcher and start reading
    //  new message.
    demux->write (msg);
    next_step (tmpbuf, 1, &bp_decoder_t::one_byte_size_ready);
}

