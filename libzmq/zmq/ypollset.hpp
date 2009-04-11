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

#ifndef __ZMQ_YPOLLSET_HPP_INCLUDED__
#define __ZMQ_YPOLLSET_HPP_INCLUDED__

#include <zmq/i_signaler.hpp>
#include <zmq/atomic_bitmap.hpp>
#include <zmq/ysemaphore.hpp>
#include <zmq/stdint.hpp>

namespace zmq
{

    //  ypollset allows for rapid polling for up to constant number of  
    //  different signals each produced by a different thread. The number of
    //  possible signals is platform-dependent.

    class ypollset_t : public i_signaler
    {
    public:

        typedef atomic_bitmap_t::integer_t integer_t;

        //  Create the pollset.
        inline ypollset_t ()
        {
        }

        //  Send a signal to the pollset (i_singnaler implementation).
        void signal (int signal_);

        //  Wait for signal. Returns a set of signals in form of a bitmap.
        //  Signal with index 0 corresponds to value 1, index 1 to value 2,
        //  index 2 to value 3 etc.
        inline integer_t poll ()
        {
            integer_t result = 0;
            while (!result) {
                result = bits.izte (integer_t (1) << wait_signal, 0);
                if (!result) {
                    sem.wait ();
                    result = bits.xchg (0);
                }

                //  If btsr was really atomic, result would never be 0 at this
                //  point, i.e. no looping would be possible. However, to
                //  support even CPU architectures without CAS instruction
                //  we allow btsr to be composed of two independent atomic
                //  operation (set and reset). In such case looping can occur
                //  sporadically.
            }
            return result;      
        }

        //  Same as poll, however, if there is no signal available,
        //  function returns zero immediately instead of waiting for a signal.
        inline integer_t check ()
        {
            return bits.xchg (0);
        }

    private:

        //  Wait signal is carried in the most significant bit of integer.
        enum {wait_signal = sizeof (integer_t) * 8 - 1};

        //  The bits of the pollset.
        atomic_bitmap_t bits;

        //  Used by thread waiting for signals to sleep if there are no
        //  signals available.
        ysemaphore_t sem;

        //  Disable copying of ypollset object.
        ypollset_t (const ypollset_t&);
        void operator = (const ypollset_t&);
    };

}

#endif
