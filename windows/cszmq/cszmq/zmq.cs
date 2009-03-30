﻿using System;
using System.Runtime.InteropServices;


    public class Zmq : IDisposable
    {
        private bool isDisposed = false;
        private IntPtr transport;

        public const int SCOPE_LOCAL = 1;
        public const int SCOPE_PROCESS = 2;
        public const int SCOPE_GLOBAL = 3;

        public const int MESSAGE_DATA = 1;
        public const int MESSAGE_GAP = 2;

        public const int STYLE_DATA_DISTRIBUTION = 1;
        public const int STYLE_LOAD_BALANCING = 2;

        //  Defines watermark level.
        public const int NO_LIMIT = -1;

        public const int NO_SWAP = 0;

        public Zmq ()
        {
            transport = IntPtr.Zero;
        }

        public Zmq (string host)
        {
            Open (host);
        }

        ~Zmq ()
        {
            Dispose (false);
        }

        public void Open (string host)
        {
            transport = zmq_create (host);
        }
	
	    public void Mask (int messageMask)
        {
            zmq_mask (transport, Convert.ToUInt32 (messageMask));
        }
	
        public int CreateExchange (string name, int scope, string location, int style)
        {
            return zmq_create_exchange (transport, name, scope, location, style);
        }

        public int CreateQueue (string name, int scope, string location, Int64 hwm, Int64 lwm, Int64 swap)
        {
            return zmq_create_queue (transport, name, scope, location, hwm, lwm, swap);
        }

        public void Bind (string exchangeName, string queueName, string exchangeArgs, string queueArgs)
        {
            zmq_bind (transport, exchangeName, queueName, exchangeArgs, queueArgs);
        }

        //class Pinner
        //{
        //    public Pinner (GCHandle aHandle)
        //    {
        //        handle = aHandle;
        //    }
        //    public GCHandle handle;
        //    public void Unpin (IntPtr ptr)
        //    {
        //        handle.Free ();
        //    }
        //};

        public bool Send (int exchange, byte[] message, bool block)
        {
            //  TODO: Commented code pins the memory down instead of copying
            //  the content. However, the performance result are undecisive.
            //  Check this out in the future.
            //
            //if (data.Length < 131072)
            //{
		
            IntPtr ptr = Marshal.AllocHGlobal (message.Length);
            Marshal.Copy (message, 0, ptr, message.Length);
		    
		    int sent = zmq_send2 (transport, exchange, ptr,
                Convert.ToUInt32 (message.Length), Convert.ToInt32 (block));
		
            //}
            //else
            //{
            //    Pinner pin = new Pinner (
            //        GCHandle.Alloc (data, GCHandleType.Pinned));
            //    try
            //    {
            //        sent = czmq_send (transport, eid, pin.handle.AddrOfPinnedObject (),
            //            Convert.ToUInt32 (data.Length), pin.Unpin, Convert.ToInt32 (block));
            //    }
            //    catch
            //    {
            //        pin.handle.Free ();
            //    }
            //}

            return Convert.ToBoolean (sent);
            
        }
	
        public int Receive (out byte[] message, out int type, bool block)
        {
            IntPtr ptr;
            UInt32 messageSize;
            UInt32 typeOut;
		    		     
            int queue = zmq_receive2 (transport, out ptr, out messageSize,
		          out typeOut, Convert.ToInt32 (block));
		    type = (int) typeOut;

            if (ptr == IntPtr.Zero)
            {
                message = null;
                return queue;
            }

            message = new byte[messageSize];
            Marshal.Copy (ptr, message, 0, message.Length);
           
            return queue;
        }

        public void Destroy ()
        {
            if (transport != IntPtr.Zero)
            {
                zmq_destroy (transport);
                transport = IntPtr.Zero;
            }
        }

        #region IDisposable Members

        public void Dispose ()
        {
            Dispose (true);
            GC.SuppressFinalize (this);
        }

        protected virtual void Dispose (bool disposing)
        {
            if (!isDisposed)
            {
                if (disposing)
                {
                    // dispose managed resources
                }
                Destroy ();
                isDisposed = true;
            }
        }

        #endregion

        #region C API

        [DllImport ("libczmq", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        static extern IntPtr zmq_create (string host);

        [DllImport ("libczmq", CallingConvention = CallingConvention.Cdecl)]
        static extern void zmq_destroy (IntPtr zmq);

        [DllImport ("libczmq", CallingConvention = CallingConvention.Cdecl)]
        static extern void zmq_mask (IntPtr zmq, UInt32 notifications);

        [DllImport ("libczmq", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        static extern int zmq_create_exchange (IntPtr zmq, string name,
            int scope, string location, int style);

        [DllImport ("libczmq", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        static extern int zmq_create_queue (IntPtr zmq, string name, int scope,
            string location, Int64 hwm, Int64 lwm, Int64 swap);

        [DllImport ("libczmq", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        static extern void zmq_bind (IntPtr zmq, string exchange_name, string queue_name,
            string exchange_options, string queue_options);

        [DllImport ("libczmq", CallingConvention = CallingConvention.Cdecl)]
        static extern int zmq_send2 (IntPtr zmq, int exchange, IntPtr data,
            UInt32 size, int block);

        [DllImport ("libczmq", CallingConvention = CallingConvention.Cdecl)]
        static extern int zmq_receive2 (IntPtr zmq, [Out] out IntPtr data,
             [Out] out UInt32 size, [Out] out UInt32 type, int block);
	
        #endregion
    }