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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
using namespace std;

#include <zmq/platform.hpp>
#ifdef ZMQ_HAVE_WINDOWS
#else
#include <unistd.h>
#include <sys/socket.h>
#ifdef ZMQ_HAVE_OPENVMS
#include <sys/types.h>
#include <sys/time.h>
#else
#include <sys/select.h>
#endif

#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#include <zmq.hpp>
#include <zmq/fd.hpp>
#include <zmq/tcp_socket.hpp>
#include <zmq/tcp_listener.hpp>
#include <zmq/server_protocol.hpp>
using namespace zmq;

//  Maps object name to object info.
typedef map <string, string> objects_t;

int main (int argc, char *argv [])
{
    //  Check command line parameters.
    if ((argc != 1 && argc != 2) || (argc == 2 &&
          strcmp (argv [1], "--help") == 0)) {
        printf ("Usage: zmq_server [port]\n");
        printf ("Default port is %d.\n", (int) default_locator_port);
        
        return 1;
    }

#ifdef ZMQ_HAVE_WINDOWS
    //  Initialise Windows socker layer.
    WORD version_requested = MAKEWORD (2, 2);
    WSADATA wsa_data;
    int rc = WSAStartup (version_requested, &wsa_data);
    errno_assert (rc == 0);
    assert (LOBYTE (wsa_data.wVersion) == 2 || HIBYTE (wsa_data.wVersion) == 2);
#endif

    //  Create a tcp_listener.
    char location [256];
    int port = (argc >= 2 ? atoi (argv [1]) : default_locator_port);
    zmq_snprintf (location, sizeof (location), "0.0.0.0:%d", port);
    tcp_listener_t listening_socket (location);
     
    //	Create list of descriptors.
    typedef vector <tcp_socket_t *> socket_list_t;
    socket_list_t socket_list;

    //  Intitialise descriptors for select.
    fd_set result_set_fds, source_set_fds, error_set_fds;
    
    FD_ZERO (&source_set_fds);
    FD_ZERO (&result_set_fds);
    FD_ZERO (&error_set_fds); 
    fd_t fd_int = listening_socket.get_fd ();
    
    FD_SET (fd_int, &source_set_fds);
    fd_t maxfdp1 = fd_int + 1;
        
    //  Object repository. Individual object maps are placed into slots
    //  identified by the type ID of particular object.
    objects_t objects [type_id_count];
    
    while (true) {
        
       //  Select on the descriptors.
       int rc = 0;
       while (rc == 0) {
           
           memcpy (&result_set_fds, &source_set_fds, sizeof (source_set_fds));
           memcpy (&error_set_fds, &source_set_fds, sizeof (source_set_fds));
	            
           rc = select (maxfdp1, &result_set_fds, NULL, &error_set_fds, NULL);
#ifdef ZMQ_HAVE_WINDOWS
           win_assert (rc != SOCKET_ERROR);
#else
           errno_assert (rc != -1);
#endif
       }    
      
       //  Traverse all the sockets.
       for (socket_list_t::size_type pos = 0; pos < socket_list.size ();
             pos ++) {
 	   
           //  Get the socket being currently being processed.
           fd_t s = socket_list [pos]->get_fd ();
           
           if (FD_ISSET (s, &error_set_fds)) {
                
                //  Delete the tcp_socket from socket_list. 
                delete socket_list [pos];
                socket_list.erase (socket_list.begin () + pos);
                
                //  Erase the whole list of file descriptors selectfds and add
                //  them back without the one erased from socket_list.
                FD_ZERO (&source_set_fds);
                FD_SET (fd_int , &source_set_fds);
                for (socket_list_t::size_type i = 0; i < socket_list.size ();
                     i ++) 
                    FD_SET (socket_list [i]->get_fd (), &source_set_fds);
                 
                continue;
            }
	    else if (FD_ISSET (s, &result_set_fds)) {
           
                //  Read command ID.
                unsigned char cmd;
                unsigned char reply;
                int nbytes = socket_list [pos]->read (&cmd, 1);

                //  Connection closed by peer.
                if (nbytes == -1 || nbytes == 0) {

                    //  Delete the tcp_socket from socket_list. 
                    delete socket_list [pos];
                    socket_list.erase (socket_list.begin () + pos);

	                //  Erase the whole list of filedescriptors selectfds
                        //  and add them back without the one erased
                        //  from socket_list.
                        FD_ZERO (&source_set_fds);
                        FD_SET (fd_int , &source_set_fds);
                        for (socket_list_t::size_type i = 0;
                              i < socket_list.size (); i ++)                            
                        FD_SET (socket_list [i]->get_fd (), &source_set_fds);

                    continue;
                }

                assert (nbytes == 1);

                switch (cmd) {
                case create_id:
                    {

                        //  Parse type ID.
                        unsigned char type_id;
                        nbytes = socket_list [pos]->read (&type_id, 1);
                        assert (nbytes == 1);

                        //  Parse object name.
                        unsigned char size;
                        nbytes = socket_list [pos]->read (&size, 1);
                        assert (nbytes == 1);
                        char name [256];
                        nbytes = socket_list [pos]->read (&name, size);
                        assert (nbytes == size);
                        name [size] = 0;

                        //  Parse location.
                        nbytes = socket_list [pos]->read (&size, 1);
                        assert (nbytes == 1);
                        char location [256];
                        nbytes = socket_list [pos]->read (&location, size);
                        assert (nbytes == size);
                        location [size] = 0;

                        //  Insert object to the repository.
                        pair<objects_t::iterator, bool> res =
                            objects [type_id].insert (
                            objects_t::value_type (name, location));
                        if (!res.second)
                           res.first->second = location;

                        //  Send reply command.
                        reply = create_ok_id;
                        nbytes = socket_list [pos]->write (&reply, 1);
                        assert (nbytes == 1);
#ifdef ZMQ_TRACE
                        printf ("Object %d:%s created (%s).\n", type_id, name,
                            location);
#endif
                        break;
                    }
                case get_id:
                    {
                        //  Parse type ID.
                        unsigned char type_id;

                        nbytes = socket_list [pos]->read (&type_id, 1);
                        assert (nbytes ==1);

                        //  Parse object name.
                        unsigned char size;
                        nbytes = socket_list [pos]->read (&size, 1);
                        assert (nbytes == 1);
                        char name [256];
                        nbytes = socket_list [pos]->read (&name, size);
                        assert (nbytes == size);
                        name [size] = 0;

                        //  Find the exchange in the repository.
                        objects_t::iterator it =
                            objects [type_id].find (name);
                        if (it == objects [type_id].end ()) {

                            //  Send the error.
                            reply = fail_id;

                            nbytes = socket_list [pos]->write (&reply, 1);                             
                            assert (nbytes == 1);
#ifdef ZMQ_TRACE
                            printf ("Error when looking for an object: "
                                "object %d:%s does not exist.\n",
                                (int) type_id, name);
#endif
                            break;
                        }

                        //  Send reply command.
                        reply = get_ok_id;
                        nbytes = socket_list [pos]->write (&reply, 1);
                        assert (nbytes == 1);

                        //  Send the location.
                        size = it->second.size ();
                        nbytes = socket_list [pos]->write (&size, 1);
                        assert (nbytes == 1);
                        nbytes = socket_list [pos]->write (
                            it->second.c_str (), size);
                        assert (nbytes == size);

#ifdef ZMQ_TRACE
                        printf ("Object %d:%s retrieved (%s).\n", (int) type_id,
                            name, info->location.c_str ());
#endif
                        break;
                    }
                default:
                    assert (false);
                }
            }

        }
	
        //  Accept incoming connection.
        if (FD_ISSET (fd_int, &result_set_fds)) {    
	        socket_list.push_back (
                    new tcp_socket_t (listening_socket, true));           
            fd_t s = socket_list.back ()->get_fd ();
            FD_SET (s, &source_set_fds);
            
            if (maxfdp1 <= s)
                maxfdp1 = s + 1;
        }

    }

#ifdef ZMQ_HAVE_WINDOWS

    //  Uninitialise Windows socket layer.
    rc = WSACleanup ();
    wsa_assert (rc != SOCKET_ERROR);
#endif
   
    return 0;

}

