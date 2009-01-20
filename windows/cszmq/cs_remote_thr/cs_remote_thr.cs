﻿using System;
using System.Collections.Generic;
using System.Text;
using zmq;
using System.Runtime.InteropServices;

namespace remote_thr
{
    class dn_remote_thr
    {
        static unsafe int Main (string[] args)
        {
            if (args.Length != 3)
            {
                Console.Out.WriteLine ("usage: remote_thr <hostname> <message-size> " +
                "<roundtrip-count>\n");
                return 1;
            }

            String host = args[0];
            uint msg_size = Convert.ToUInt32 (args[1]);
            int num_msg = Convert.ToInt32 (args[2]);

            //  Exchange id.
            int ex;

            //  Create 0MQ Dnzmq class
            Dnzmq w = new Dnzmq (host);
            
            //  Create 0MQ exchnge.           
            ex = w.create_exchange ("E", Dnzmq.ZMQ_SCOPE_LOCAL, host);

            //  Bind.
            w.bind ("E", "Q");

            //  Allocate memory for messages.
            byte[] msg = new byte[msg_size];

            //  Start sending messages.
            for (int i = 0; i < num_msg + 1; i++)
                w.send (ex, msg, msg_size);

            System.Threading.Thread.Sleep (5000);
            return 0;
        }
    }
}

