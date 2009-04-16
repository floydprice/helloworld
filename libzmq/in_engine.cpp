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

#include <zmq/in_engine.hpp>
#include <zmq/err.hpp>

zmq::in_engine_t *zmq::in_engine_t::create (int64_t hwm_, int64_t lwm_,
    uint64_t swap_size_)
{
    in_engine_t *instance = new in_engine_t (hwm_, lwm_, swap_size_);
    zmq_assert (instance);
    return instance;
}

zmq::in_engine_t::in_engine_t (int64_t hwm_, int64_t lwm_,
      int64_t swap_size_) :
    hwm (hwm_),
    lwm (lwm_),
    swap_size (swap_size_)
{
}

zmq::in_engine_t::~in_engine_t ()
{
}

void zmq::in_engine_t::subscribe (const char *criteria_)
{
    mux.subscribe (criteria_);
}

bool zmq::in_engine_t::read (message_t *msg_)
{
    return mux.read (msg_);
}

void zmq::in_engine_t::get_watermarks (int64_t *hwm_, int64_t *lwm_)
{
    *hwm_ = hwm;
    *lwm_ = lwm;
}

int64_t zmq::in_engine_t::get_swap_size ()
{
    return swap_size;
}
