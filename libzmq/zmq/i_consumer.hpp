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

#ifndef __ZMQ_I_CONSUMER_HPP_INCLUDED__
#define __ZMQ_I_CONSUMER_HPP_INCLUDED__

namespace zmq
{

    //  Interface to be imeplemented by message consumers that
    //  use the i_mux interface to retrieve messages.
    class i_consumer
    {
    public:

        virtual ~i_consumer () {};

        //  Notify the message consumer it can resume receiving messages.
        virtual void revive () = 0;

        //  Notify the message consumer a new pipe was attached to its mux.
        virtual void receive_from () = 0;
    };

}

#endif
