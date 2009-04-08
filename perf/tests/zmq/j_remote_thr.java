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

class j_remote_thr
{
     public static void main (String [] args)
     {
         if (args.length != 3) {
             System.out.println ("usage: java j_remote_thr <hostname> " +
                 "<message size> <message count>");
             return;
         }

         //  Parse the command line arguments.
         String host = args [0];
         int messageSize = Integer.parseInt (args [1]);
         int messageCount = Integer.parseInt (args [2]);

         //  Initialise 0MQ runtime.
         Zmq obj = new Zmq (host);

         //  Create the wiring.
         int eid = obj.createExchange ("EL", Zmq.SCOPE_LOCAL, null,
             Zmq.STYLE_LOAD_BALANCING);
         obj.bind ("EL", "QG", null, null);
         
         //  Send the messages to LocalThr.
         for (int i = 0; i != messageCount; i ++) {
             byte data [] = new byte [messageSize];
             obj.send (eid, data, true);
         }

         //  Wait a while to be sure that all the messages ware actually sent.
         try {
             Thread.sleep (5000);
         }
         catch (InterruptedException e) {
             e.printStackTrace ();
         }
    }
}
