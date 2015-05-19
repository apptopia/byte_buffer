/*
 * Copyright (C) 2015 Apptopia Inc.
 * You may redistribute this under the terms of the MIT license.
 * See LICENSE for details
 */

#include "ruby.h"

void
Init_byte_buffer_ext()
{
    VALUE mByteBuffer, cBuffer;

    mByteBuffer  = rb_define_module_under(rb_cObject, "ByteBuffer");
    cBuffer      = rb_define_class_under(mByteBuffer, "BufferLol", rb_cObject);
}
