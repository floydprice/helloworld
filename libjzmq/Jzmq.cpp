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

#include <zmq/Jzmq.h>

#include <stdio.h>
#include <assert.h>

#include <zmq.hpp>

static jfieldID context_fid = NULL;

struct context_t
{
    zmq::locator_t *locator;
    zmq::dispatcher_t *dispatcher;
    zmq::i_thread *io_thread;
    zmq::api_thread_t *api_thread;
};


JNIEXPORT void JNICALL Java_Jzmq_construct (JNIEnv *env, jobject obj,
    jstring hostname_)
{
    //  Cache context field ID.
    jclass cls = env->GetObjectClass (obj);
    context_fid = env->GetFieldID (cls, "context", "J");

    //  Get the hostname.
    assert (hostname_);
    char *hostname = (char*) env->GetStringUTFChars (hostname_, 0);
    assert (hostname);

    //  Create the context.
    context_t *context = new context_t;
    assert (context);
    context->dispatcher = new zmq::dispatcher_t (2);
    assert (context->dispatcher);
    context->locator = new zmq::locator_t (hostname);
    assert (context->locator);
    context->io_thread = zmq::select_thread_t::create (context->dispatcher);
    assert (context->io_thread);
    context->api_thread = zmq::api_thread_t::create (context->dispatcher,
        context->locator);
    assert (context->api_thread);

    //  Clean-up.
    env->ReleaseStringUTFChars (hostname_, hostname);

    //  Set the context.
    env->SetLongField (obj, context_fid, (jlong) context);
}

JNIEXPORT void JNICALL Java_Jzmq_finalize (JNIEnv *env, jobject obj)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);

    //  Deallocate the 0MQ infrastructure.
    delete context->locator;
    delete context->dispatcher;   
    delete context;
}

JNIEXPORT void JNICALL Java_Jzmq_mask (JNIEnv *env, jobject obj,
    int message_mask_)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);
    
    //  Forward the call.
    context->api_thread->mask (message_mask_);

}

JNIEXPORT jint JNICALL Java_Jzmq_createExchange (JNIEnv *env, jobject obj,
    jstring exchange_, jint scope_, jstring nic_, jint style_)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);

    //  Get the exchange name.
    assert (exchange_);
    char *exchange = (char*) env->GetStringUTFChars (exchange_, 0);
    assert (exchange);

    //  Get NIC name.
    char *nic = NULL;
    if (nic_)
        nic = (char*) env->GetStringUTFChars (nic_, 0);

    //  Get the scope.
    zmq::scope_t scope = zmq::scope_local;
    if (scope_ == Jzmq_SCOPE_GLOBAL)
        scope = zmq::scope_global;

    //  Get the style.
    zmq::style_t style = zmq::style_load_balancing;
    if (style_ == Jzmq_STYLE_DATA_DISTRIBUTION)
        style = zmq::style_data_distribution;

    jint eid = context->api_thread->create_exchange (exchange, scope, nic,
        context->io_thread, 1, &context->io_thread, style);

    //  Clean-up.
    env->ReleaseStringUTFChars (exchange_, exchange);
    if (nic)
        env->ReleaseStringUTFChars (nic_, nic);

    return eid;
}

JNIEXPORT jint JNICALL Java_Jzmq_createQueue (JNIEnv *env, jobject obj,
    jstring queue_, jint scope_, jstring nic_, jlong hwm_, jlong lwm_, 
    jlong swap_size_)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);

    //  Get the queue name.
    assert (queue_);
    char *queue = (char*) env->GetStringUTFChars (queue_, 0);
    assert (queue);

    //  Get NIC name.
    char *nic = NULL;
    if (nic_)
        nic = (char*) env->GetStringUTFChars (nic_, 0);

    //  Get the scope.
    zmq::scope_t scope = zmq::scope_local;
    if (scope_ == Jzmq_SCOPE_GLOBAL)
        scope = zmq::scope_global;

    int64_t hwm = (int64_t) hwm_;
    int64_t lwm = (int64_t) lwm_;
    int64_t swap_size = (int64_t) swap_size_;

    jint qid = context->api_thread->create_queue (queue, scope, nic,
        context->io_thread, 1, &context->io_thread, hwm, lwm, swap_size);

    //  Clean-up.
    env->ReleaseStringUTFChars (queue_, queue);
    if (nic)
        env->ReleaseStringUTFChars (nic_, nic);

    return qid;
}

JNIEXPORT void JNICALL Java_Jzmq_bind (JNIEnv *env, jobject obj,
    jstring exchange_, jstring queue_, jstring exchange_arguments_,
    jstring queue_arguments_)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);

    //  Get the exchange name.
    assert (exchange_);
    char *exchange = (char*) env->GetStringUTFChars (exchange_, 0);
    assert (exchange);

    //  Get the queue name.
    assert (queue_);
    char *queue = (char*) env->GetStringUTFChars (queue_, 0);
    assert (queue);

    char *exchange_arguments = (char*) env->GetStringUTFChars (exchange_arguments_, 0);
    assert (exchange_arguments);

    char *queue_arguments = (char*) env->GetStringUTFChars (queue_arguments_, 0);
    assert (queue_arguments);

    context->api_thread->bind (exchange, queue,
        context->io_thread, context->io_thread, exchange_arguments,
        queue_arguments);

    //  Clean-up.
    env->ReleaseStringUTFChars (exchange_, exchange);
    env->ReleaseStringUTFChars (queue_, queue);
}

JNIEXPORT void JNICALL Java_Jzmq_send (JNIEnv *env, jobject obj,
    jint eid_, jbyteArray data_)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);

    //  Get the data from the bytearray.
    jsize size = env->GetArrayLength (data_); 
    jbyte *data = env->GetByteArrayElements (data_, 0);

    //  Create the message.
    zmq::message_t msg (size);
    memcpy (msg.data (), data, size);

    //  Release the bytearray.
    env->ReleaseByteArrayElements (data_, data, 0);

    //  Send the message.
    context->api_thread->send (eid_, msg);
}

JNIEXPORT jbyteArray JNICALL Java_Jzmq_receive (JNIEnv *env, jobject obj)
{
    //  Get the context.
    context_t *context = (context_t*) env->GetLongField (obj, context_fid);
    assert (context);

    //  Get new message.
    zmq::message_t msg;
    context->api_thread->receive (&msg);

    //  Move the message to the byte array.
    jbyteArray result = env->NewByteArray (msg.size ());
    env->SetByteArrayRegion (result, 0, msg.size (), (jbyte*) msg.data ());

    return result;
}
