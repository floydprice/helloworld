!
!     Copyright (c) 2007-2009 FastMQ Inc.
!
!     This file is part of 0MQ.
!
!     0MQ is free software; you can redistribute it and/or modify it under
!     the terms of the Lesser GNU General Public License as published by
!     the Free Software Foundation; either version 3 of the License, or
!     (at your option) any later version.
!
!     0MQ is distributed in the hope that it will be useful,
!     but WITHOUT ANY WARRANTY; without even the implied warranty of
!     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!     Lesser GNU General Public License for more details.
!
!     You should have received a copy of the Lesser GNU General Public License
!     along with this program.  If not, see <http://www.gnu.org/licenses/>.
!

      program REMOTE LAT

      implicit none

      parameter 			ZMQ$_SCOPE_LOCAL  = 1
      parameter 			ZMQ$_SCOPE_GLOBAL = 3
      parameter 			ZMQ$_STYLE_DATA_DISTRIBUTION = 1
      parameter 			ZMQ$_STYLE_LOAD_BALANCING = 2
      parameter 			ZMQ$_NO_LIMIT = -1
      parameter 			ZMQ$_NO_SWAP = 0

      external 				ZMQ_CREATE
      external 				ZMQ_CREATE_EXCHANGE
      external 				ZMQ_CREATE_QUEUE
      external 				ZMQ_BIND
      external 				ZMQ_SEND
      external 				ZMQ_RECEIVE
      external 				ZMQ_DESTROY
      external 				ZMQ_FREE

      external 				lib$get_vm, lib$free_vm

      integer*4 			ZMQ_CREATE
      integer*4 			ZMQ_CREATE_EXCHANGE
      integer*4 			ZMQ_CREATE_QUEUE
      integer*4 			ZMQ_BIND
      integer*4 			ZMQ_SEND
      integer*4 			ZMQ_RECEIVE
      integer*4 			ZMQ_DESTROY
      integer*4 			ZMQ_FREE

      integer*4 			lib$get_vm, lib$free_vm

      integer*4 			okay

      integer*4 			nmsg

      integer*4 			ilen
      integer*4 			olen
      integer*4 			obuf

      character*60   			host
      character*60   			e_iface
      character*60   			q_iface
      character*80 			args

      integer*4 			i

      integer*4    			obj
      integer*4 			eid
      integer*4 			qid



      call lib$get_foreign(args)

      read(args, *) host, q_iface, e_iface, ilen, nmsg

      if (ilen .gt. 8192) then
         print *, 'Message size must be less than or equal to 8192B.'
         call exit
      end if


      okay = ZMQ_CREATE(%descr(host), obj)

      if (.not. okay) then
         call lib$stop(%val(okay))
      end if

      okay = ZMQ_CREATE_EXCHANGE(%val(obj), %descr('EG'),
     +%val(ZMQ$_SCOPE_GLOBAL), %descr(e_iface),
     +%val(ZMQ$_STYLE_DATA_DISTRIBUTION), eid)

      if (.not. okay) then
         call lib$stop(%val(okay))
      end if

      okay = ZMQ_CREATE_QUEUE(%val(obj), %descr('QG'),
     +%val(ZMQ$_SCOPE_GLOBAL), %descr(q_iface),
     +%val(ZMQ$_NO_LIMIT),
     +%val(ZMQ$_NO_LIMIT), %val(ZMQ$_NO_SWAP), qid)

      if (.not. okay) then
         call lib$stop(%val(okay))
      end if

      do i = 1, nmsg

         okay = ZMQ_RECEIVE(%val(obj), obuf, olen, %val(0), %val(1))

         if (.not. okay) then
            call lib$stop(%val(okay))
         end if

!  Here we should check message size %val(olen) == ilen.

        okay = ZMQ_SEND(%val(obj), %val(eid), obuf, %val(olen),
     +%val(1))

         if (.not. okay) then
            call lib$stop(%val(okay))
         end if

         okay = ZMQ_FREE (%val(obuf))

         if (.not. okay) then
            call lib$stop(%val(okay))
         end if

      end do

!  Give some time to 0MQ to send last message.
      call decc$sleep(%val(1)) ! Sleep for 1 second

      okay = ZMQ_DESTROY(%val(obj))

      if (.not. okay) then
         call lib$stop(%val(okay))
      end if

      end

