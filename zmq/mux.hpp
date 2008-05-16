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

#ifndef __ZMQ_MUX_HPP_INCLUDED__
#define __ZMQ_MUX_HPP_INCLUDED__

#include <assert.h>
#include <vector>

#include "pipe.hpp"

namespace zmq
{

    class mux_t
    {
    public:

        mux_t ();
        ~mux_t ();

        //  Adds a pipe to receive messages from
        void receive_from (pipe_t *pipe_);

        //  Returns a message, NULL if no message is available
        void *read ();

    private:

        typedef std::vector <pipe_t*> pipes_t;
        pipes_t pipes;
        int current;
    };

}

#endif
