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


#ifndef __ZMQ_BP_DECODER_HPP_INCLUDED__
#define __ZMQ_BP_DECODER_HPP_INCLUDED__

#include <string>

#include <zmq/demux.hpp>
#include <zmq/decoder.hpp>
#include <zmq/message.hpp>

namespace zmq
{
    //  Decoder for 0MQ backend protocol. Converts data batches into messages.

    class bp_decoder_t : public decoder_t <bp_decoder_t>
    {
    public:

        bp_decoder_t (demux_t *demux_, bool receive_identity_,
            std::string *identity_);

        //  Clears any partially decoded messages.
        void reset ();

    private:

        bool one_byte_size_ready ();
        bool eight_byte_size_ready ();
        bool message_ready ();

        demux_t *demux;
        bool receive_identity;
        std::string *identity;
        bool startup;
        unsigned char tmpbuf [8];
        message_t message;

        bp_decoder_t (const bp_decoder_t&);
        void operator = (const bp_decoder_t&);
    };

}

#endif
