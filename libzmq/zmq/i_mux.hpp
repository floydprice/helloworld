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

#ifndef __ZMQ_I_MUX_HPP_INCLUDED__
#define __ZMQ_I_MUX_HPP_INCLUDED__

#include <zmq/i_source.hpp>
#include <zmq/pipe.hpp>
#include <zmq/i_consumer.hpp>

namespace zmq
{

    class i_mux : public i_source
    {
    public:

        virtual ~i_mux () {};

        //  Returns high and low watermarks for a specified mux. These are
        //  used to derive high and low watermarks for pipes attached to
        //  the mux. When hwm is zero, the pipe will be of infinite capacity.
        virtual void get_watermarks (int64_t &hwm_, int64_t &lwm_) = 0;

        //  Returns the swap size.
        virtual int64_t get_swap_size () = 0;

        //  Associate the specified engine with the mux.
        virtual void register_consumer (i_consumer *consumer_) = 0;

        //  Adds a pipe to receive messages from.
        virtual void receive_from (pipe_t *pipe_) = 0;

        //  Revives a stalled pipe.
        virtual void revive (pipe_t *pipe_) = 0;

        //  Returns true if there are no pipes attached.
        virtual bool empty () = 0;

        //  Drop references to the specified pipe.
        virtual void release_pipe (pipe_t *pipe_) = 0;

        //  Initiate shutdown of all associated pipes.
        virtual void initialise_shutdown () = 0;
    };

}

#endif
