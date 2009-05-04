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

#ifndef __ZMQ_IN_ENGINE_HPP_INCLUDED__
#define __ZMQ_IN_ENGINE_HPP_INCLUDED__

#include <zmq/i_engine.hpp>
#include <zmq/mux.hpp>

namespace zmq
{

    class in_engine_t : public i_engine
    {
    public:

        static in_engine_t *create (mux_t *mux_, int64_t hwm_, int64_t lwm_,
            uint64_t swap_size_);

        bool read (message_t *msg_);

        //  i_engine implementation.
        void get_watermarks (int64_t *hwm_, int64_t *lwm_);
        int64_t get_swap_size ();

    protected:
        //  i_engine implementation.
        i_pollable *cast_to_pollable ();
        const char *get_arguments ();
        void revive (pipe_t *pipe_);
        void head (pipe_t *pipe_, int64_t position_);
        void send_to (pipe_t *pipe_);
        void receive_from (pipe_t *pipe_);
        void terminate_pipe (pipe_t *pipe_);
        void terminate_pipe_ack (pipe_t *pipe_);

    private:

        in_engine_t (mux_t *mux_, int64_t hwm_, int64_t lwm_, 
            int64_t swap_size_);
        ~in_engine_t ();

        mux_t *mux;
        int64_t hwm;
        int64_t lwm;
        int64_t swap_size;
    };
}
#endif
