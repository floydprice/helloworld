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

#ifndef __ZMQ_BP_ENGINE_HPP_INCLUDED__
#define __ZMQ_BP_ENGINE_HPP_INCLUDED__

#include "i_engine.hpp"
#include "bp_encoder.hpp"
#include "bp_decoder.hpp"
#include "tcp_socket.hpp"

namespace zmq
{

    class bp_engine_t : public i_engine
    {
    public:

        bp_engine_t (dispatcher_t *dispatcher_, int thread_id_,
            bool listen_, const char *address_, uint16_t port_,
            int source_thread_id_, int destination_thread_id_,
            size_t writebuf_size_, size_t readbuf_size_);
        ~bp_engine_t ();

        void set_signaler (i_signaler *signaler_);
        void revive (int thread_id_);

        int get_fd ();
        short get_events ();

        void in_event ();
        void out_event ();

    private:

        dispatcher_proxy_t proxy;   

        unsigned char *writebuf;
        size_t writebuf_size;

        unsigned char *readbuf;
        size_t readbuf_size;

        size_t write_size;
        size_t write_pos;

        bp_encoder_t encoder;
        bp_decoder_t decoder;
        tcp_socket_t socket;

        short events;
    };

}

#endif
