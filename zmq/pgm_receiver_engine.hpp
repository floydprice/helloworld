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

#ifndef __ZMQ_PGM_RECEIVER_ENGINE_HPP_INCLUDED__
#define __ZMQ_PGM_RECEIVER_ENGINE_HPP_INCLUDED__

#include "i_pollable.hpp"
#include "bp_decoder.hpp"
#include "epgm_socket.hpp"

namespace zmq
{

    class pgm_receiver_engine_t : public i_pollable
    {
    public:

        pgm_receiver_engine_t (dispatcher_t *dispatcher_, int engine_id_,
            const char *network_, uint16_t port_, size_t readbuf_size_, 
            int destination_engine_id_);
        ~pgm_receiver_engine_t ();

        //  i_pollable interface implementation
        void set_signaler (i_signaler *signaler_);
        void revive (pollfd *pfd_, int count_, int engine_id_);
        int get_fd_count ();
        int get_pfds (pollfd *pfd_, int count_);
        void in_event (pollfd *pfd_, int count_, int index_);
        void out_event (pollfd *pfd_, int count_, int index_);

    private:

        dispatcher_proxy_t proxy; 

        bp_decoder_t decoder;
        epgm_socket_t epgm_socket;

        iovec *iov;
        size_t iov_len;
    };

}

#endif
