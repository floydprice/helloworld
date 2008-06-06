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

#include <cstdio>

#include "../../transports/zmq.hpp"
#include "../scenarios/lat.hpp"

int main (int argc, char *argv [])
{

    if (argc != 5) {
        printf ("Usage: remote <global_locator IP> <global_locator port> "
            "<message size> <message count>\n");
        return 1;
    }

    perf::zmq_t transport (true, "QIN", "EOUT", argv [1], atoi (argv [2]),
        NULL, 0);

    perf::remote_lat ( &transport, atoi (argv [3]), atoi (argv [4])); 

    return 0;
}
