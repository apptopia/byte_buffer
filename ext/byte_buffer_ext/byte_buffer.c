/*
 * Copyright (C) 2015 Apptopia Inc.
 * You may redistribute this under the terms of the MIT license.
 * See LICENSE for details
 */

#include "ruby.h"
#include <string.h>
#include "portable_endian.h"

#define BYTE_BUFFER_EMBEDDED_SIZE 1024

static VALUE rb_byte_buffer_allocate(VALUE klass);
static VALUE rb_byte_buffer_initialize(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_length(VALUE self);
static VALUE rb_byte_buffer_append(VALUE self, VALUE str);
static VALUE rb_byte_buffer_append_int(VALUE self, VALUE i);
static VALUE rb_byte_buffer_discard(VALUE self, VALUE n);
static VALUE rb_byte_buffer_read(VALUE self, VALUE n);
static VALUE rb_byte_buffer_read_int(VALUE self);
static VALUE rb_byte_buffer_read_short(VALUE self);
static VALUE rb_byte_buffer_read_byte(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_to_str(VALUE self);
static VALUE rb_byte_buffer_inspect(VALUE self);

static void byte_buffer_free(void *ptr);
static size_t byte_buffer_memsize(const void *ptr);

static const rb_data_type_t buffer_data_type = {
    "byte_buffer/buffer",
    {NULL, byte_buffer_free, byte_buffer_memsize}
};

typedef struct {
    size_t size;
    size_t write_pos;
    size_t read_pos;
    char   embedded_buffer[BYTE_BUFFER_EMBEDDED_SIZE];
    char   *b_ptr;
} buffer_t;

#define READ_PTR(buffer_ptr) \
    (buffer_ptr->b_ptr + buffer_ptr->read_pos)

#define READ_SIZE(buffer_ptr) \
    (buffer_ptr->write_pos - buffer_ptr->read_pos)

#define WRITE_PTR(buffer_ptr) \
    (buffer_ptr->b_ptr + buffer_ptr->write_pos)

#define ENSURE_WRITE_CAPACITY(buffer_ptr,len) \
    { if (buffer_ptr->write_pos + len > buffer_ptr->size) grow_buffer(buffer_ptr, len); }

#define ENSURE_READ_CAPACITY(buffer_ptr,len) \
    { if (buffer_ptr->read_pos + len > buffer_ptr->write_pos) \
        rb_raise(rb_eRangeError, "%zu bytes requred, but only %zu available", (size_t)len, READ_SIZE(buffer_ptr)); }

static uint32_t value_to_uint32(VALUE x);
static void grow_buffer(buffer_t* buffer_ptr, size_t len);

void
Init_byte_buffer_ext()
{
    VALUE rb_mByteBuffer, rb_cBuffer;

    rb_mByteBuffer  = rb_define_module("ByteBuffer");
    rb_cBuffer      = rb_define_class_under(rb_mByteBuffer, "Buffer", rb_cObject);

    rb_define_alloc_func(rb_cBuffer, rb_byte_buffer_allocate);
    rb_define_method(rb_cBuffer, "initialize", rb_byte_buffer_initialize, -1);
    rb_define_method(rb_cBuffer, "length", rb_byte_buffer_length, 0);
    rb_define_method(rb_cBuffer, "append", rb_byte_buffer_append, 1);
    rb_define_method(rb_cBuffer, "append_int", rb_byte_buffer_append_int, 1);
    rb_define_method(rb_cBuffer, "discard", rb_byte_buffer_discard, 1);
    rb_define_method(rb_cBuffer, "read", rb_byte_buffer_read, 1);
    rb_define_method(rb_cBuffer, "read_int", rb_byte_buffer_read_int, 0);
    rb_define_method(rb_cBuffer, "read_short", rb_byte_buffer_read_short, 0);
    rb_define_method(rb_cBuffer, "read_byte", rb_byte_buffer_read_byte, -1);
    rb_define_method(rb_cBuffer, "to_str", rb_byte_buffer_to_str, 0);
    rb_define_method(rb_cBuffer, "inspect", rb_byte_buffer_inspect, 0);
}

VALUE
rb_byte_buffer_allocate(VALUE klass)
{
    buffer_t *b;
    VALUE obj = TypedData_Make_Struct(klass, buffer_t, &buffer_data_type, b);
    b->b_ptr = b->embedded_buffer;
    b->size  = BYTE_BUFFER_EMBEDDED_SIZE;

    return obj;
}

VALUE
rb_byte_buffer_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE str, prealloc_size;

    rb_scan_args(argc, argv, "02", &str, &prealloc_size);

    if (!NIL_P(str))
        rb_byte_buffer_append(self, str);

    return self;
}

VALUE
rb_byte_buffer_length(VALUE self)
{
    buffer_t *b;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);

    return UINT2NUM(READ_SIZE(b));
}

VALUE
rb_byte_buffer_append(VALUE self, VALUE str)
{
    char     *c_str;
    size_t   len;
    buffer_t *b;


    Check_Type(str, T_STRING);
    c_str = RSTRING_PTR(str);
    len   = RSTRING_LEN(str);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, len);
    memcpy(WRITE_PTR(b), c_str, len);
    b->write_pos += len;

    return self;
}

VALUE
rb_byte_buffer_append_int(VALUE self, VALUE i)
{
    buffer_t *b;
    uint32_t i32 = value_to_uint32(i);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 4);
    i32 = htobe32(i32);
    memcpy(WRITE_PTR(b), &i32, 4);
    b->write_pos += 4;

    return self;
}

VALUE
rb_byte_buffer_discard(VALUE self, VALUE n)
{
    buffer_t *b;
    long len;

    Check_Type(n, T_FIXNUM);
    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    len = FIX2LONG(n);
    if (len < 0) rb_raise(rb_eRangeError, "Cannot discard a negative number of bytes");
    ENSURE_READ_CAPACITY(b, len);
    b->read_pos += len;

    return self;
}

VALUE
rb_byte_buffer_read(VALUE self, VALUE n)
{
    buffer_t *b;
    long len;
    VALUE str;

    Check_Type(n, T_FIXNUM);
    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    len = FIX2LONG(n);
    if (len < 0) rb_raise(rb_eRangeError, "Cannot read a negative number of bytes");
    ENSURE_READ_CAPACITY(b, len);
    str = rb_str_new(READ_PTR(b), len);
    b->read_pos += len;

    return str;
}

VALUE
rb_byte_buffer_read_int(VALUE self)
{
    buffer_t *b;
    uint32_t i32;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 4);
    i32 = be32toh(*((uint32_t*)READ_PTR(b)));
    b->read_pos += 4;

    return UINT2NUM(i32);
}

VALUE
rb_byte_buffer_read_short(VALUE self)
{
    buffer_t *b;
    uint16_t i16;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 2);
    i16 = be16toh(*((uint16_t*)READ_PTR(b)));
    b->read_pos += 2;

    return UINT2NUM(i16);
}

VALUE
rb_byte_buffer_read_byte(int argc, VALUE *argv, VALUE self)
{
    VALUE f_signed;
    buffer_t *b;
    uint8_t i8;

    rb_scan_args(argc, argv, "01", &f_signed);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 1);
    i8 = *((uint8_t*)READ_PTR(b));
    b->read_pos += 1;

    if (RTEST(f_signed))
        return INT2NUM((int8_t)i8);
    else
        return UINT2NUM(i8);
}

VALUE
rb_byte_buffer_to_str(VALUE self)
{
    buffer_t *b;
    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);

    return rb_str_new(READ_PTR(b), READ_SIZE(b));
}

VALUE
rb_byte_buffer_inspect(VALUE self)
{
    buffer_t *b;
    VALUE str;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    str = rb_sprintf("#<%s:%p read_pos:%zu write_pos:%zu len:%zu capacity:%zu>",
        rb_obj_classname(self), (void*)self, b->read_pos, b->write_pos, READ_SIZE(b), b->size);

    return str;
}

static uint32_t
value_to_uint32(VALUE x)
{
    if (FIXNUM_P(x))
        return FIX2ULONG(x);
    else
        rb_raise(rb_eTypeError, "expected `interger', got %s ", rb_obj_classname(x));

    return 0;
}

void
grow_buffer(buffer_t* buffer_ptr, size_t len)
{
    size_t new_size = buffer_ptr->write_pos - buffer_ptr->read_pos + len;

    if (new_size <= buffer_ptr->size) {
        memcpy(buffer_ptr->b_ptr, READ_PTR(buffer_ptr), READ_SIZE(buffer_ptr));
        buffer_ptr->write_pos -= buffer_ptr->read_pos;
        buffer_ptr->read_pos = 0;
    } else {
        char *new_b_ptr;

        new_size += new_size / 2;
        new_b_ptr = ALLOC_N(char, new_size);
        memcpy(new_b_ptr, READ_PTR(buffer_ptr), READ_SIZE(buffer_ptr));
        if (buffer_ptr->b_ptr != buffer_ptr->embedded_buffer) xfree(buffer_ptr->b_ptr);
        buffer_ptr->b_ptr = new_b_ptr;
        buffer_ptr->size = new_size;
        buffer_ptr->write_pos -= buffer_ptr->read_pos;
        buffer_ptr->read_pos = 0;
    }
}

void
byte_buffer_free(void *ptr)
{
    buffer_t *b = ptr;
    if (b->b_ptr != b->embedded_buffer) xfree(b->b_ptr);
    xfree(b);
}

size_t
byte_buffer_memsize(const void *ptr)
{
    return ptr ? sizeof(buffer_t) : 0;
}
