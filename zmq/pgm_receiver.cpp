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

#include "pgm_receiver.hpp"
#include "err.hpp"

zmq::pgm_receiver_t::pgm_receiver_t (const char *network_,
    uint16_t port_): g_transport (NULL)
{
    printf ("GLIB: %i.%i.%i\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
    
    pgm_init ();

    pgm_gsi_t gsi;

    int rc = pgm_create_md5_gsi (&gsi);
	assert (rc == 0);

    struct pgm_sock_mreq recv_smr, send_smr;
    int smr_len = 1;
	rc = pgm_if_parse_transport (network_, AF_INET, &recv_smr, &send_smr, &smr_len);
    assert (rc == 0);
	assert (smr_len == 1);

    rc = pgm_transport_create (&g_transport, &gsi, port_, &recv_smr, 1, &send_smr);
    errno_assert (rc == 0);

    int g_max_tpdu = 1500;
    int g_sqns = 10;

    pgm_transport_set_max_tpdu (g_transport, g_max_tpdu);
	pgm_transport_set_txw_sqns (g_transport, g_sqns);
	pgm_transport_set_rxw_sqns (g_transport, g_sqns);
    pgm_transport_set_hops (g_transport, 16);
	pgm_transport_set_ambient_spm (g_transport, 8192*1000);
    guint spm_heartbeat[] = { 1*1000, 1*1000, 2*1000, 4*1000, 8*1000, 16*1000, 
        32*1000, 64*1000, 128*1000, 256*1000, 512*1000, 1024*1000, 2048*1000, 4096*1000, 8192*1000 };

    pgm_transport_set_heartbeat_spm (g_transport, spm_heartbeat, G_N_ELEMENTS(spm_heartbeat));
    pgm_transport_set_peer_expiry (g_transport, 5*8192*1000);
    pgm_transport_set_spmr_expiry (g_transport, 250*1000);
    pgm_transport_set_nak_bo_ivl (g_transport, 50*1000);
    pgm_transport_set_nak_rpt_ivl (g_transport, 200*1000);
    pgm_transport_set_nak_rdata_ivl (g_transport, 200*1000);
    pgm_transport_set_nak_data_retries (g_transport, 5);
    pgm_transport_set_nak_ncf_retries (g_transport, 2);

    // Do not create transmit window and sdo not send SPM broadcast
    pgm_transport_set_recv_only (g_transport, FALSE);

    rc = pgm_transport_bind (g_transport);
    errno_assert (rc == 0);

    printf ("TSI: %s\n", pgm_print_tsi (&g_transport->tsi));

}

zmq::pgm_receiver_t::~pgm_receiver_t ()
{
    pgm_transport_destroy (g_transport, TRUE);
}

int zmq::pgm_receiver_t::get_fd (int *fds_, int nfds_)
{
    int nfds = IP_MAX_MEMBERSHIPS;
    pollfd fds [IP_MAX_MEMBERSHIPS];
    memset (fds, '\0', sizeof (fds));
    int rc = pgm_transport_poll_info (g_transport, (pollfd*)&fds, &nfds, EPOLLIN);

    // has to return two fds
    assert (rc == 2);

    assert (rc <= nfds_);

    printf ("pgm_transport_poll_info %i, %s(%i)\n", rc, __FILE__, __LINE__);

    for (int i = 0; i < rc; i++) {
        fds_[i] = fds[i].fd;
    }

    return nfds;
}

size_t zmq::pgm_receiver_t::read (unsigned char *data_, size_t size_)
{ 
    ssize_t nbytes = pgm_transport_recv (g_transport, data_, size_, 0 /*MSG_DONTWAIT*/);
    errno_assert (nbytes != -1);
    return (size_t) nbytes;
}

size_t zmq::pgm_receiver_t::read_msg (iovec **iov_)
{
    printf ("%s(%i)\n", __FILE__, __LINE__);
    ssize_t nbytes = pgm_transport_recvmsg (g_transport, &msgv, 0);
    errno_assert (nbytes != -1);
//    printf ("received %i bytes, ", (int)nbytes);
//    printf ("in %i iovecs, %s(%i)\n", (int)msgv.msgv_iovlen, __FILE__, __LINE__);
//    fflush (stdout);    

    // iov
    *iov_ = msgv.msgv_iov;

    return (size_t)nbytes;
}