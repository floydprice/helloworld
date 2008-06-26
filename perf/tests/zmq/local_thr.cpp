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

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "../../transports/zmq.hpp"
#include "../scenarios/thr.hpp"

using namespace std;

int main (int argc, char *argv [])
{
    if (argc != 8){
        cerr << "Usage: local_thr <global_locator IP> <global_locator port> "
            << "<listen IP> <listen port> <message size> <message count> "
            << "<number of threads>\n";
        return 1;
    }

    const char *g_locator = argv [1];
    unsigned short g_locator_port = atoi (argv [2]);

    const char *listen_ip = argv [3];
    unsigned short listen_port = atoi (argv [4]);

    int thread_count = atoi (argv [7]);
    size_t msg_size = atoi (argv [5]);
    int roundtrip_count = atoi (argv [6]);

    cout << "threads: " << thread_count << endl;
    cout << "message size: " << msg_size << endl;
    cout << "roundtrip count: " << roundtrip_count << endl;

    // create array of transports
    perf::i_transport **transports = new perf::i_transport* [thread_count];

    for (int thread_nbr = 0; thread_nbr < thread_count; thread_nbr++)
    {
        string queue_name ("Q");
        queue_name += perf::to_string (thread_nbr);

        string exchange_name ("E");
        exchange_name += perf::to_string (thread_nbr);

        transports [thread_nbr] = new perf::zmq_t (false, queue_name.c_str (), 
            exchange_name.c_str (), g_locator,g_locator_port, 
            listen_ip, listen_port + 2 * thread_nbr);
    }

    perf::local_thr (transports, msg_size, roundtrip_count, thread_count);
    
    for (int thread_nbr = 0; thread_nbr < thread_count; thread_nbr++)
    {
        delete transports [thread_nbr];
    }
    
    delete [] transports;

    return 0;
}
