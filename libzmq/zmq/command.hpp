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

#ifndef __ZMQ_COMMAND_HPP_INCLUDED__
#define __ZMQ_COMMAND_HPP_INCLUDED__

#include <string.h>

#include <zmq/stdint.hpp>
#include <zmq/i_engine.hpp>
#include <zmq/i_pollable.hpp>
#include <zmq/pipe.hpp>
#include <zmq/formatting.hpp>

namespace zmq
{

    //  This structure defines all the commands that can be sent to an engine.

    class i_engine;

    struct engine_command_t
    {
        enum type_t
        {
            send_to,
            receive_from
        } type;

        union {
            struct {
                class pipe_t *pipe;
            } send_to;
            struct {
                class pipe_t *pipe;
            } receive_from;
        } args;   
    };

    //  This structure defines all the commands that can be sent to a thread.
    //  It also provides 'constructors' for all the commands.

    struct command_t
    {
        enum type_t
        {
            stop,
            revive_reader,
            notify_writer,
            terminate_pipe_req,
            terminate_pipe_ack,
            register_pollable,
            unregister_pollable,
            engine_command
        } type;

        union
        {
            struct {
            } stop;
            struct {
                class pipe_t *pipe;
            } revive_reader;
            struct {
                class pipe_t *pipe;
                uint64_t position;
            } notify_writer;
            struct {
                class pipe_t *pipe;
            } terminate_pipe_req;
            struct {
                class pipe_t *pipe;
            } terminate_pipe_ack;
            struct {
                i_pollable *pollable;
            } register_pollable;
            struct {
                i_pollable *pollable;
            } unregister_pollable;
            struct {
                i_engine *engine;
                engine_command_t command;
            } engine_command;
        } args;

        inline void init_stop ()
        {
            type = stop;
        }

        inline void init_revive_reader (class pipe_t *pipe_)
        {
            type = revive_reader;
            args.revive_reader.pipe = pipe_;
        }

        inline void init_notify_writer (class pipe_t *pipe_, uint64_t position_)
        {
            type = notify_writer;
            args.notify_writer.pipe = pipe_;
            args.notify_writer.position = position_;
        }

        inline void init_terminate_pipe_req (class pipe_t *pipe_)
        {
            type = terminate_pipe_req;
            args.terminate_pipe_req.pipe = pipe_;
        }

        inline void init_terminate_pipe_ack (class pipe_t *pipe_)
        {
            type = terminate_pipe_ack;
            args.terminate_pipe_ack.pipe = pipe_;
        }

        inline void init_register_pollable (i_pollable *pollable_)
        {
            type = register_pollable;
            args.register_pollable.pollable = pollable_;
        }

        inline void init_unregister_pollable (i_pollable *pollable_)
        {
            type = unregister_pollable;
            args.unregister_pollable.pollable = pollable_;
        }

        inline void init_engine_send_to (i_engine *engine_, pipe_t *pipe_)
        {
            type = engine_command;
            args.engine_command.engine = engine_;
            args.engine_command.command.type = engine_command_t::send_to;
            args.engine_command.command.args.send_to.pipe = pipe_;
        }

        inline void init_engine_receive_from (i_engine *engine_, pipe_t *pipe_)
        {
            type = engine_command;
            args.engine_command.engine = engine_;
            args.engine_command.command.type = engine_command_t::receive_from;
            args.engine_command.command.args.receive_from.pipe = pipe_;
        }

    };

}    

#endif
