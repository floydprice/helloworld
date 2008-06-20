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

#include <cstdlib>
#include <iostream>
#include "../../transports/zmq.hpp"
#include "../scenarios/fo.hpp"

using namespace std;

int main (int argc, char *argv [])
{
    if (argc != 8) {
        cerr << "Usage: local_fo <global_locator IP> <global_locator port> "
            << "<listen IP> <listen port> <message size> <message count> "
            << "<number of subscribers>\n";
        return 1;
    }

    const char *g_locator = argv [1];
    unsigned short g_locator_port = atoi (argv [2]);

    const char *listen_ip = argv [3];
    unsigned short listen_port = atoi (argv [4]);

    size_t msg_size = atoi (argv [5]);
    int roundtrip_count = atoi (argv [6]);
    int subs_count = atoi (argv [7]);

    cout << "subcribers: " << subs_count << endl;
    cout << "message size: " << msg_size << endl;
    cout << "roundtrip count: " << roundtrip_count << endl;

    perf::zmq_t transport (false, "QIN", "EOUT", g_locator, g_locator_port, 
        listen_ip, listen_port);
   
    perf::local_fo (&transport, msg_size, roundtrip_count, subs_count);

    return 0;
}
