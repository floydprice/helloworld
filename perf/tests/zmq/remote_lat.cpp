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
#include <iostream>

#include "../../transports/zmq.hpp"
#include "../scenarios/lat.hpp"

using namespace std;

int main (int argc, char *argv [])
{

    if (argc != 5) {
        cerr << "Usage: remote <global_locator IP> <global_locator port> "
            << "<message size> <roundtrip count>" << endl;
        return 1;
    }

    // Parse & print command line arguments
    const char *g_locator_ip = argv [1];
    unsigned short g_locator_port = atoi (argv [2]);

    size_t msg_size = atoi (argv [3]);
    int roundtrip_count = atoi (argv [4]);

    cout << "message size: " << msg_size << " [B]" << endl;
    cout << "roundtrip count: " << roundtrip_count << endl;

    // Create zmq transport
    perf::zmq_t transport (true, "QIN", "EOUT", g_locator_ip, g_locator_port,
        NULL, 0);

    // Do the job, for more detailed info refer to ../scenarios/lat.hpp
    perf::remote_lat (&transport, msg_size, roundtrip_count); 

    return 0;
}
