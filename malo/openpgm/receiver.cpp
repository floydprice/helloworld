#include <assert.h>
#include <cstdio>
#include <cstdlib>

#include <unistd.h>

#include "../../zmq/dispatcher.hpp"
#include "../../zmq/api_engine.hpp"
#include "../../zmq/poll_thread.hpp"
#include "../../zmq/pgm_receiver_engine.hpp"

int main (int argc, char *argv [])
{
    if (argc < 4) {
        printf ("usage: receiver network port nloops\n");
        return 1;
    }

    const char *network = argv [1];
    int port = atoi (argv [2]);

    int nloops = atoi (argv [3]);

    printf ("network \"%s\", port %i, nloops %i\n", 
        network, port, nloops);

    zmq::dispatcher_t disptacher (2);

    zmq::api_engine_t api (&disptacher, 0);
    zmq::pgm_receiver_engine_t engine (&disptacher, 1, network, port, /*1456*/8192, 0);
    zmq::poll_thread_t io (&engine);

    zmq::cmsg_t msg;
    
    char buff [1024];
    memset (buff, '\0', sizeof (buff));

    for (int i = 0; i < nloops; i++) {
        api.receive (&msg);

        assert (msg.size <= sizeof (buff));
        
        memset (buff, '\0', sizeof (buff));
        memcpy (buff, msg.data, msg.size);
        printf ("%i, \'%s\' size %i\n\n", i, buff, (int)msg.size);
        zmq::free_cmsg (msg);
    }

    printf ("loop finished, %s(%i)\n", __FILE__, __LINE__);
    return 0;
}
