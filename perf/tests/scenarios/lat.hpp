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

/*

    Message flow diagram

          'local'                   'remote'
      (started first)          (started second)
             |
             |                        
             |                         |
             | sync message (size 1B)  | 
             |<------------------------|
             |                         |
             |      message (size)     |
             |------------------------>|
             |<------------------------|
             |                         |
             |      message (size)     |
             |------------------------>|
             |<------------------------|
             |                         |
             ...    message count
             |                         |
             | sync message (size 1B)  |
             |------------------------>|
      resuls gathering                 v
        computations
             |
             v
*/

#ifndef __PERF_LAT_HPP_INCLUDED__
#define __PERF_LAT_HPP_INCLUDED__

#include <iostream>
#include <fstream>
#include "../../transports/i_transport.hpp"
#include "../../helpers/time.hpp"

namespace perf
{
    //  Function sends roundtrip_count messages of msg_size and
    //  receives them back, one by one eg. send-receive, send-receive.
    //  Peer has to echo messages of the same size. Timestamp 
    //  at the beginign and the end of transport is captured and
    //  therefore transport latency can be calculated.
    void local_lat (i_transport *transport_, size_t msg_size_, 
        int roundtrip_count_, bool raw_ = false)
    {
        //  Wait for 'remote' side 1B sync message.
        size_t size = transport_->receive ();
        assert (size == 1);

        //  We got the sync message. This means the remote_lat process
        //  has already bound its exchange to our queue. Before we start
        //  message exchange, however, we must make sure the remote_lat
        //  process also binds its queue to our exchange. This
        //  is necessary to guarantee that all echoed messages will get
        //  delivered. As there is no such synchronisation mechanism
        //  available now, simply wait 1 second.
#ifdef ZMQ_HAVE_WINDOWS
        Sleep (1000);
#else
        sleep (1);
#endif

        //  Prepare arrays for raw test
        time_instant_t *raw_start_times = NULL;
        time_instant_t *raw_stop_times = NULL;

        if (raw_) {
            raw_start_times = new time_instant_t [roundtrip_count_];
            raw_stop_times = new time_instant_t [roundtrip_count_];
        }
        
        //  Capture timestamp at the begining of the test.
        time_instant_t start_time = now ();

        for (int msg_nbr = 0; msg_nbr < roundtrip_count_; msg_nbr++) {
            //  In raw mode capture timestamp before each message round
            if (raw_)
                raw_start_times [msg_nbr] = now ();

            //  Send test message.
            transport_->send (msg_size_);

            //  Receive echoed message.
            size_t size = transport_->receive ();

            //  Check incoming message size.
            assert (size == msg_size_);

            //  In raw mode capture stop timestamp
            if (raw_)
                raw_stop_times [msg_nbr] = now ();
        }
        
        //  Capture the end timestamp of the test.
        time_instant_t stop_time = now ();

        //  Set 2 fixed decimal places.
        std::cout.setf(std::ios::fixed);
        std::cout.precision (2);

        //  Calculate & print results.
        uint64_t test_time = (uint64_t) (stop_time - start_time);
        double latency = (double) (test_time / 2000) / 
            (double) roundtrip_count_;

        std::cout <<  "Your average latency is " << latency 
            << " [us]" << std::endl << std::endl;

        //  In raw more save time diffs
        if (raw_) {
            //  Open time for raw results
            std::ofstream raw_outf ("raw_tests.dat", 
                std::ios::out | std::ios::app);
            assert (raw_outf.is_open ());
            
            raw_outf.precision (2);

            for (int msg_nbr = 0; msg_nbr < roundtrip_count_; msg_nbr++) {
                raw_outf << std::fixed << std::noshowpoint;
                raw_outf << (raw_stop_times [msg_nbr] 
                    - raw_start_times [msg_nbr]) / 2;
                raw_outf << std::endl;
            }

            //  Close raw output file
            raw_outf.close ();

            delete [] raw_start_times;
            delete [] raw_stop_times;
        }

        //  Save the results into tests.dat file.
        std::ofstream outf ("tests.dat", std::ios::out | std::ios::app);
        assert (outf.is_open ());
        
        outf.precision (2);

        //  Output file format, separate line for each run is appended 
        //  to the tests.dat file.
        //
        // 0, roundtrip count, msg size [B], test time [ms], latency [us]
        //
        outf << std::fixed << std::noshowpoint << "0,"
            << roundtrip_count_ << "," << msg_size_ << "," 
            << test_time / (uint64_t) 1000000<< "," << latency << std::endl;
        
        outf.close ();

        //  Send sync message to the peer.
        transport_->send (1);
    }

    //  Function recevies messages and sends the same size messages back.
    //  Receive-send (echo) procedure is performed roundtrip_count times.
    void remote_lat (i_transport *transport_, size_t msg_size_, 
        int roundtrip_count_)
    {
        //  Send sync message to peer.
        transport_->send (1);

        for (int msg_nbr = 0; msg_nbr < roundtrip_count_; msg_nbr++)
        {
            //  Receive message.
            size_t size = transport_->receive ();

            //  Check incoming message size.
            assert (size == msg_size_);
            
            //  Send it back.
            transport_->send (size); 
        }
        
        //  Wait for sync message.
        size_t size = transport_->receive ();
        assert (size == 1);
    }
}
#endif
