/*
    Copyright (c) 2007-2008 FastMQ Inc.

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

#include <zmq/engine_base.hpp>

namespace zmq
{

    class in_engine_t : public engine_base_t <false, true>
    {
    public:

        ZMQ_EXPORT static in_engine_t *create (uint64_t hwm_, uint64_t lwm_);

        ZMQ_EXPORT bool read (message_t *msg_);

        //  i_engine implementation.
        void get_watermarks (uint64_t *hwm_, uint64_t *lwm_);

    private:

        in_engine_t (uint64_t hwm_, uint64_t lwm_);
        ~in_engine_t ();

        uint64_t hwm;
        uint64_t lwm;
    };

}

#endif
