/*
 * Copyright (C) 2015 Apptopia Inc.
 * You may redistribute this under the terms of the MIT license.
 * See LICENSE for details
 */

#include "ruby.h"
#include <string.h>

static VALUE rb_byte_buffer_allocate(VALUE klass);
static VALUE rb_byte_buffer_lol_int(VALUE self);
static VALUE rb_byte_buffer_lol_str(VALUE self);

static void byte_buffer_free(void *ptr);
static size_t byte_buffer_memsize(const void *ptr);

static const rb_data_type_t buffer_data_type = {
    "byte_buffer/buffer",
    {NULL, byte_buffer_free, byte_buffer_memsize}
};

typedef struct {
    size_t size;
    size_t read_pos;
    char   embedded_buffer[1024];
} buffer_t;

void
Init_byte_buffer_ext()
{
    VALUE rb_mByteBuffer, rb_cBuffer;

    rb_mByteBuffer  = rb_define_module("ByteBuffer");
    rb_cBuffer      = rb_define_class_under(rb_mByteBuffer, "BufferLol", rb_cObject);

    rb_define_alloc_func(rb_cBuffer, rb_byte_buffer_allocate);
    rb_define_method(rb_cBuffer, "lol_int", rb_byte_buffer_lol_int, 0);
    rb_define_method(rb_cBuffer, "lol_str", rb_byte_buffer_lol_str, 0);
}

VALUE
rb_byte_buffer_allocate(VALUE klass)
{
    buffer_t *b;
    VALUE obj = TypedData_Make_Struct(klass, buffer_t, &buffer_data_type, b);
    memcpy(b->embedded_buffer, "wee-haa", strlen("wee-haa"));

    return obj;
}

VALUE
rb_byte_buffer_lol_int(VALUE self)
{
    return INT2NUM(42);
}

VALUE
rb_byte_buffer_lol_str(VALUE self)
{
    buffer_t *b;
    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);

    return rb_str_new_cstr(b->embedded_buffer);
}

void
byte_buffer_free(void *ptr)
{
    buffer_t *b = ptr;
    xfree(b);
}

size_t
byte_buffer_memsize(const void *ptr)
{
    return ptr ? sizeof(buffer_t) : 0;
}
