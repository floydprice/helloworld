/*
    Copyright (c) 2007 FastMQ Inc.

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

#include "bp_encoder.hpp"
#include "wire.hpp"

zmq::bp_encoder_t::bp_encoder_t (dispatcher_proxy_t *proxy_,
      int source_thread_id_) :
    proxy (proxy_),
    source_thread_id (source_thread_id_)
{
    init_cmsg (msg);
    next_step (NULL, 0, &bp_encoder_t::message_ready);
}

zmq::bp_encoder_t::~bp_encoder_t ()
{
    free_cmsg (msg);
}

bool zmq::bp_encoder_t::size_ready ()
{
    next_step (msg.data, msg.size, &bp_encoder_t::message_ready);
    return true;
}

bool zmq::bp_encoder_t::message_ready ()
{
    free_cmsg (msg);
    init_cmsg (msg);
    if (!proxy->read (source_thread_id, &msg))
        return false;

    if (msg.size < 255) {
        tmpbuf [0] = (unsigned char) msg.size;
        next_step (tmpbuf, 1, &bp_encoder_t::size_ready);
    }
    else {
        tmpbuf [0] = 0xff;
        put_uint64 (tmpbuf + 1, msg.size);
        next_step (tmpbuf, 9, &bp_encoder_t::size_ready);
    }
    return true;
}
