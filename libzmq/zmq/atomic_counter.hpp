/*
    Copyright (c) 2007-2008 FastMQ Inc.

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


#ifndef __ZMQ_ATOMIC_COUNTER_HPP_INCLUDED__
#define __ZMQ_ATOMIC_COUNTER_HPP_INCLUDED__

#include <zmq/err.hpp>
#include <zmq/stdint.hpp>
#include <zmq/platform.hpp>
#include <zmq/mutex.hpp>

#if defined ZMQ_FORCE_MUTEXES
#define ZMQ_ATOMIC_COUNTER_MUTEX
#elif (defined __i386__ || defined __x86_64__) && defined __GNUC__
#define ZMQ_ATOMIC_COUNTER_X86
#elif 0 && defined __sparc__ && defined __GNUC__
#define ZMQ_ATOMIC_COUNTER_SPARC
#elif defined ZMQ_HAVE_WINDOWS
#define ZMQ_ATOMIC_COUNTER_WINDOWS
#elif defined ZMQ_HAVE_SOLARIS
#define ZMQ_ATOMIC_COUNTER_SOLARIS
#else
#define ZMQ_ATOMIC_COUNTER_MUTEX
#endif

#if defined ZMQ_ATOMIC_COUNTER_WINDOWS
#include <zmq/windows.hpp>
#elif defined ZMQ_ATOMIC_COUNTER_SOLARIS
#include <atomic.h>
#endif

namespace zmq
{

    //  This class represents an integer that can be incremented/decremented
    //  in atomic fashion.

    class atomic_counter_t
    {
    public:

        typedef uint32_t integer_t;

        inline atomic_counter_t (integer_t value_ = 0) :
            value (value_)
        {
        }

        inline ~atomic_counter_t ()
        {
        }

        //  Set counter value (not thread-safe).
        inline void set (integer_t value_)
        {
            value = value_;
        }

        //  Atomic addition. Returns false if counter was zero
        //  before the operation.
        inline bool add (integer_t increment)
        {
#if defined ZMQ_ATOMIC_COUNTER_WINDOWS
            integer_t old = InterlockedExchangeAdd ((LONG*) &value,
                increment);
            return old != 0;
#elif defined ZMQ_ATOMIC_COUNTER_SOLARIS
            integer_t nv = atomic_add_32_nv (&value, increment);
            return nv != increment;             
#elif defined ZMQ_ATOMIC_COUNTER_X86
            volatile integer_t *val = &value;
            __asm__ volatile ("lock; xaddl %0,%1"
                : "=r" (increment), "=m" (*val)
                : "0" (increment), "m" (*val)
                : "cc");
            return increment;
#elif defined ZMQ_ATOMIC_COUNTER_SPARC
            volatile integer_t *val = &value;
            integer_t tmp;
            integer_t result;
            __asm__ volatile(
                "ld [%4], %1\n\t"
                "1:\n\t"
                "add %1, %0, %2\n\t"
                "cas [%4], %1, %2\n\t"
                "cmp %1, %2\n\t"
                "bne,a,pn %%icc, 1b\n\t"
                "mov %2, %1\n\t"
                : "+r" (increment), "=&r" (tmp), "=&r" (result), "+m" (*val)
                : "r" (val)
                : "cc");
            return result; 
#elif defined ZMQ_ATOMIC_COUNTER_MUTEX
            sync.lock ();
            bool result = value ? true : false;
            value += increment;
            sync.unlock ();
            return result;
#else
#error
#endif
        }

        //  Atomic subtraction. Returns false if the counter drops to zero.
        inline bool sub (integer_t decrement)
        {
#if defined ZMQ_ATOMIC_COUNTER_WINDOWS
            LONG delta = - ((LONG) decrement);
            integer_t old = InterlockedExchangeAdd ((LONG*) &value, delta);
            return old - decrement != 0;
#elif defined ZMQ_ATOMIC_COUNTER_SOLARIS
            int32_t delta = - ((int32_t) decrement);
            integer_t nv = atomic_add_32_nv (&value, delta);
            return nv != 0;
#elif defined ZMQ_ATOMIC_COUNTER_X86
            integer_t oldval = -decrement;
            volatile integer_t *val = &value;
            __asm__ volatile ("lock; xaddl %0,%1"
                : "=r" (oldval), "=m" (*val)
                : "0" (oldval), "m" (*val)
                : "cc");
            return oldval != decrement;
#elif defined ZMQ_ATOMIC_COUNTER_SPARC
            volatile integer_t *val = &value;
            integer_t tmp;
            integer_t result;
            __asm__ volatile(
                "ld [%4], %1\n\t"
                "1:\n\t"
                "add %1, %0, %2\n\t"
                "cas [%4], %1, %2\n\t"
                "cmp %1, %2\n\t"
                "bne,a,pn %%icc, 1b\n\t"
                "mov %2, %1\n\t"
                : "+r" (-decrement), "=&r" (tmp), "=&r" (result), "+m" (*val)
                : "r" (val)
                : "cc");
            return result <= decrement;
#elif defined ZMQ_ATOMIC_COUNTER_MUTEX
            sync.lock ();
            value -= decrement;
            bool result = value ? true : false;
            sync.unlock ();
            return result;
#else
#error
#endif
        }

    private:

        volatile integer_t value;
#if defined ZMQ_ATOMIC_COUNTER_MUTEX
        mutex_t sync;
#endif

        atomic_counter_t (const atomic_counter_t&);
        void operator = (const atomic_counter_t&);
    };

}

//  Remove macros local to this file.
#if defined ZMQ_ATOMIC_COUNTER_WINDOWS
#undef ZMQ_ATOMIC_COUNTER_WINDOWS
#endif
#if defined ZMQ_ATOMIC_COUNTER_SOLARIS
#undef ZMQ_ATOMIC_COUNTER_SOLARIS
#endif
#if defined ZMQ_ATOMIC_COUNTER_X86
#undef ZMQ_ATOMIC_COUNTER_X86
#endif
#if defined ZMQ_ATOMIC_COUNTER_SPARC
#undef ZMQ_ATOMIC_COUNTER_SPARC
#endif
#if defined ZMQ_ATOMIC_COUNTER_MUTEX
#undef ZMQ_ATOMIC_COUNTER_MUTEX
#endif

#endif
