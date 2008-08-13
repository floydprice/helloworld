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

#ifndef __ZMQ_TCP_LISTENER_HPP_INCLUDED__
#define __ZMQ_TCP_LISTENER_HPP_INCLUDED__

#include "stdint.hpp"

namespace zmq
{
    //  The class encapsulating simple TCP listening socket.

    class tcp_listener_t
    {
    public:

        //  Create TCP listining socket.
        tcp_listener_t (const char *host_, const char *default_address_,
            const char *default_port_);

        //  Get the file descriptor to poll on to get notified about
        //  newly created connections.
        inline int get_fd ()
        {
            return s;
        }

        //  Accept the new connection.
        int accept ();

        int get_name (char* buf, int len);

    private:

        //  Underlying socket.
        int s;
    };

}

#endif
