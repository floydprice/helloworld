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

#ifndef __PERF_RAW_SENDER_HPP_INCLUDED__
#define __PERF_RAW_SENDER_HPP_INCLUDED__

#include "../interfaces/i_worker.hpp"

#include <stdio.h>

namespace perf
{

    class raw_sender_t : public i_worker
    {
    public:
        inline raw_sender_t (int message_count, size_t message_size) :
            message_count (message_count), message_size (message_size)
        {
        }

        inline virtual void run (i_transport &transport,
            const char *prefix = NULL)
        {
            
            //  Send the messages as quickly as possible
            for (int message_nbr = 0; message_nbr != message_count;
                  message_nbr++) {
                transport.send (message_size);
            }

        }

    protected:
        int message_count;
        size_t message_size;
    };

}

#endif
