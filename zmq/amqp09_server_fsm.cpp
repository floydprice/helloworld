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

#include "amqp09_server_fsm.hpp"

zmq::amqp09_server_fsm_t::amqp09_server_fsm_t (i_context *context_,
      tcp_socket_t *socket_, amqp09_marshaller_t *marshaller_,
      amqp09_engine_t <amqp09_server_fsm_t> *engine_,
      locator_t *locator_) :
    context (context_),
    marshaller (marshaller_),
    engine (engine_),
    locator (locator_),
    queue_id (0)
{
    //  Read AMQP protocol header from the supplied socket. This is done in
    //  a blocking manner. The rest of AMQP communication is done in
    //  non-blocking fashion.
    unsigned char buf [8];
    socket_->blocking_read (buf, 8);
    assert (buf [0] == 'A');
    assert (buf [1] == 'M');
    assert (buf [2] == 'Q');
    assert (buf [3] == 'P');
    assert (buf [4] == 1);
    assert (buf [5] == 1);
    assert (buf [6] == 0);
    assert (buf [7] == 9);

    marshaller->connection_start (0, 9, i_amqp09::field_table_t (),
        i_amqp09::longstr_t ("PLAIN", 5), i_amqp09::longstr_t ("en_US", 5));

    state = expect_connection_start_ok;
}

void zmq::amqp09_server_fsm_t::connection_start_ok (
    const i_amqp09::field_table_t &client_properties_,
    const i_amqp09::shortstr_t mechanism_,
    const i_amqp09::longstr_t response_,
    const i_amqp09::shortstr_t locale_)
{
    assert (state == expect_connection_start_ok);

    //  TODO:  Check the mechanism, the locale and credentials
    //  TODO:  SASL challenge/response handshaking may happen here

    marshaller->connection_tune (1, i_amqp09::frame_min_size, 0);
    state = expect_connection_tune_ok;
}

void zmq::amqp09_server_fsm_t::connection_tune_ok (
    uint16_t channel_max_,
    uint32_t frame_max_,
    uint16_t heartbeat_)
{
    assert (state = expect_connection_tune_ok);

    //  TODO: multiple channels, different frame sizes and heartbeats
    //  should be supported
    assert (channel_max_ == 1);
    assert (frame_max_ == i_amqp09::frame_min_size);
    assert (heartbeat_ == 0);

    state = expect_connection_open;
}

void zmq::amqp09_server_fsm_t::connection_open (
    const i_amqp09::shortstr_t virtual_host_,
    const i_amqp09::shortstr_t capabilities_,
    bool insist_)
{
    assert (state == expect_connection_open);

    //  TODO: allow different virtual hosts
    assert (virtual_host_.size == 1 && virtual_host_.data [0] == '/');

    marshaller->connection_open_ok ("");
    state = expect_channel_open;
}

void zmq::amqp09_server_fsm_t::channel_open (
    const i_amqp09::shortstr_t out_of_band_)
{
    assert (state == expect_channel_open);

    marshaller->channel_open_ok (i_amqp09::longstr_t ("0", 1));
    state = active;
    engine->flow (true);
}

void zmq::amqp09_server_fsm_t::exchange_declare (
    uint16_t ticket_,
    const i_amqp09::shortstr_t exchange_,
    const i_amqp09::shortstr_t type_,
    bool passive_,
    bool durable_,
    bool auto_delete_,
    bool internal_,
    bool nowait_,
    const i_amqp09::field_table_t &arguments_)
{
    assert (state == active);

    std::string exchange (exchange_.data, exchange_.size);
    std::string type (type_.data, type_.size);

    //  Only fanout exchange type is supported at the moment
    assert (type == "fanout");

    locator->create_exchange (exchange.c_str (), context, engine, scope_process,
        NULL, 0, NULL, 0, NULL);

    if (!nowait_)
        marshaller->exchange_declare_ok ();
}

void zmq::amqp09_server_fsm_t::queue_declare (
    uint16_t ticket_,
    const i_amqp09::shortstr_t queue_,
    bool passive_,
    bool durable_,
    bool exclusive_,
    bool auto_delete_,
    bool nowait_,
    const i_amqp09::field_table_t &arguments_)
{
    assert (state == active);

    //  Generate auto queue name if required
    //  TODO: Names should be unique process-wide!
    std::string queue;
    if (!queue_.size) {
        char buff [32];
        sprintf (buff, "#%d", queue_id);
        queue = buff;
        queue_id ++;
    }
    else
        queue = std::string (queue_.data, queue_.size);

    locator->create_queue (queue.c_str (), context, engine, scope_process,
        NULL, 0, NULL, 0, NULL);

    if (!nowait_)
        marshaller->queue_declare_ok (queue.c_str (), 0, 0);
}

void zmq::amqp09_server_fsm_t::queue_bind (
    uint16_t ticket_,
    const i_amqp09::shortstr_t queue_,
    const i_amqp09::shortstr_t exchange_,
    const i_amqp09::shortstr_t routing_key_,
    bool nowait_,
    const i_amqp09::field_table_t &arguments_)
{
    assert (state == active);

assert (false);

    //  TODO: This should do something...
    if (!nowait_)
        marshaller->queue_bind_ok ();
}

void zmq::amqp09_server_fsm_t::basic_consume (
    uint16_t ticket_,
    const i_amqp09::shortstr_t queue_,
    const i_amqp09::shortstr_t consumer_tag_,
    bool no_local_,
    bool no_ack_,
    bool exclusive_,
    bool nowait_,
    const i_amqp09::field_table_t &filter_)
{
    assert (state == active);

assert (false);

    //  TODO: This should do something...
    if (!nowait_)
        marshaller->basic_consume_ok ("");
}

void zmq::amqp09_server_fsm_t::channel_close (
    uint16_t reply_code_,
    const i_amqp09::shortstr_t reply_text_,
    uint16_t class_id_,
    uint16_t method_id_)
{
    assert (state == active);

    marshaller->channel_close_ok ();
    state = expect_connection_close;

    //  TODO: engine->flow (false);
}

void zmq::amqp09_server_fsm_t::connection_close (
    uint16_t reply_code_,
    const i_amqp09::shortstr_t reply_text_,
    uint16_t class_id_,
    uint16_t method_id_)
{
    //  TODO: this is a fake connection close
    //  Nothing is actually deallocated
    marshaller->connection_close_ok ();

    //  TODO: engine->flow (false);
}

void zmq::amqp09_server_fsm_t::unexpected ()
{
    assert (0);
}
