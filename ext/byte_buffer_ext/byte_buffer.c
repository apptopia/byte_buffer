/*
 * Copyright (C) 2015 Apptopia Inc.
 * You may redistribute this under the terms of the MIT license.
 * See LICENSE for details
 */

#include "ruby.h"
#include <string.h>
#include "portable_endian.h"

#define BYTE_BUFFER_EMBEDDED_SIZE 512

static VALUE rb_byte_buffer_allocate(VALUE klass);
static VALUE rb_byte_buffer_initialize(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_capacity(VALUE self);
static VALUE rb_byte_buffer_length(VALUE self);
static VALUE rb_byte_buffer_append(VALUE self, VALUE str);
static VALUE rb_byte_buffer_append_long(VALUE self, VALUE i);
static VALUE rb_byte_buffer_append_int(VALUE self, VALUE i);
static VALUE rb_byte_buffer_append_byte(VALUE self, VALUE i);
static VALUE rb_byte_buffer_append_short(VALUE self, VALUE i);
static VALUE rb_byte_buffer_append_double(VALUE self, VALUE i);
static VALUE rb_byte_buffer_append_float(VALUE self, VALUE i);
static VALUE rb_byte_buffer_append_byte_array(VALUE self, VALUE ary);
static VALUE rb_byte_buffer_discard(VALUE self, VALUE n);
static VALUE rb_byte_buffer_read(VALUE self, VALUE n);
static VALUE rb_byte_buffer_read_long(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_read_int(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_read_short(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_read_byte(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_read_double(VALUE self);
static VALUE rb_byte_buffer_read_float(VALUE self);
static VALUE rb_byte_buffer_read_byte_array(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_index(int argc, VALUE *argv, VALUE self);
static VALUE rb_byte_buffer_update(VALUE self, VALUE location, VALUE bytes);
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

static int32_t value_to_int32(VALUE x);
static int64_t value_to_int64(VALUE x);
static double value_to_dbl(VALUE x);
static void grow_buffer(buffer_t* buffer_ptr, size_t len);

static VALUE rb_cBuffer = 0;

void
Init_byte_buffer_ext()
{
    VALUE rb_mByteBuffer;

    rb_mByteBuffer  = rb_define_module("ByteBuffer");
    rb_cBuffer      = rb_define_class_under(rb_mByteBuffer, "Buffer", rb_cObject);

    rb_define_alloc_func(rb_cBuffer, rb_byte_buffer_allocate);
    rb_define_const(rb_cBuffer, "DEFAULT_PREALLOC_SIZE", INT2FIX(BYTE_BUFFER_EMBEDDED_SIZE));
    rb_define_method(rb_cBuffer, "initialize", rb_byte_buffer_initialize, -1);
    rb_define_method(rb_cBuffer, "capacity", rb_byte_buffer_capacity, 0);
    rb_define_method(rb_cBuffer, "length", rb_byte_buffer_length, 0);
    rb_define_method(rb_cBuffer, "append", rb_byte_buffer_append, 1);
    rb_define_method(rb_cBuffer, "append_long", rb_byte_buffer_append_long, 1);
    rb_define_method(rb_cBuffer, "append_int", rb_byte_buffer_append_int, 1);
    rb_define_method(rb_cBuffer, "append_byte", rb_byte_buffer_append_byte, 1);
    rb_define_method(rb_cBuffer, "append_short", rb_byte_buffer_append_short, 1);
    rb_define_method(rb_cBuffer, "append_double", rb_byte_buffer_append_double, 1);
    rb_define_method(rb_cBuffer, "append_float", rb_byte_buffer_append_float, 1);
    rb_define_method(rb_cBuffer, "append_byte_array", rb_byte_buffer_append_byte_array, 1);
    rb_define_method(rb_cBuffer, "discard", rb_byte_buffer_discard, 1);
    rb_define_method(rb_cBuffer, "read", rb_byte_buffer_read, 1);
    rb_define_method(rb_cBuffer, "read_long", rb_byte_buffer_read_long, -1);
    rb_define_method(rb_cBuffer, "read_int", rb_byte_buffer_read_int, -1);
    rb_define_method(rb_cBuffer, "read_short", rb_byte_buffer_read_short, -1);
    rb_define_method(rb_cBuffer, "read_byte", rb_byte_buffer_read_byte, -1);
    rb_define_method(rb_cBuffer, "read_double", rb_byte_buffer_read_double, 0);
    rb_define_method(rb_cBuffer, "read_float", rb_byte_buffer_read_float, 0);
    rb_define_method(rb_cBuffer, "read_byte_array", rb_byte_buffer_read_byte_array, -1);
    rb_define_method(rb_cBuffer, "index", rb_byte_buffer_index, -1);
    rb_define_method(rb_cBuffer, "update", rb_byte_buffer_update, 2);
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

    if (!NIL_P(prealloc_size)) {
        buffer_t *b;
        long len;

        Check_Type(prealloc_size, T_FIXNUM);
        len = FIX2LONG(prealloc_size);
        if (len < 0) rb_raise(rb_eRangeError, "prealloc size can't be negative");
        TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);

        if (len > b->size) {
            if (b->b_ptr != b->embedded_buffer) xfree(b->b_ptr);
            b->b_ptr = ALLOC_N(char, len);
            b->size = len;
            b->read_pos = b->write_pos = 0;
        }
    }

    if (!NIL_P(str))
        rb_byte_buffer_append(self, str);

    return self;
}

VALUE
rb_byte_buffer_capacity(VALUE self)
{
    buffer_t *b;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);

    return UINT2NUM(b->size);
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

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);

    if (CLASS_OF(str) == rb_cString) {
        c_str = RSTRING_PTR(str);
        len   = RSTRING_LEN(str);
    } else if (rb_cBuffer && rb_obj_is_kind_of(str, rb_cBuffer)) {
        buffer_t *other_b;
        TypedData_Get_Struct(str, buffer_t, &buffer_data_type, other_b);
        c_str = READ_PTR(other_b);
        len   = READ_SIZE(other_b);
    } else
        rb_raise(rb_eTypeError, "wrong argument type %s (expected String or %s)", rb_obj_classname(str), rb_obj_classname(self));

    ENSURE_WRITE_CAPACITY(b, len);
    memcpy(WRITE_PTR(b), c_str, len);
    b->write_pos += len;

    return self;
}

VALUE
rb_byte_buffer_append_long(VALUE self, VALUE i)
{
    buffer_t *b;
    int64_t i64 = value_to_int64(i);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 8);
    i64 = htobe64(i64);
    *((int64_t*)WRITE_PTR(b)) = i64;
    b->write_pos += 8;

    return self;
}

VALUE
rb_byte_buffer_append_int(VALUE self, VALUE i)
{
    buffer_t *b;
    int32_t i32 = value_to_int32(i);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 4);
    i32 = htobe32(i32);
    *((int32_t*)WRITE_PTR(b)) = i32;
    b->write_pos += 4;

    return self;
}

VALUE
rb_byte_buffer_append_byte(VALUE self, VALUE i)
{
    buffer_t *b;
    int32_t i32 = value_to_int32(i);
    int8_t i8 = (int8_t)i32;

    if (i32 > 0xFF || -i32 > 0x80)
        rb_raise(rb_eRangeError, "Number %d doesn't fit into byte", i32);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 1);
    *((int8_t*)WRITE_PTR(b)) = i8;
    b->write_pos += 1;

    return self;
}

VALUE
rb_byte_buffer_append_short(VALUE self, VALUE i)
{
    buffer_t *b;
    int32_t i32 = value_to_int32(i);
    int16_t i16 = (int16_t)i32;

    if (i32 > 0xFFFF || -i32 > 0x8000)
        rb_raise(rb_eRangeError, "Number %d doesn't fit into 2 bytes", i32);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 2);
    i16 = htobe16(i16);
    *((int16_t*)WRITE_PTR(b)) = i16;
    b->write_pos += 2;

    return self;
}

VALUE
rb_byte_buffer_append_double(VALUE self, VALUE i)
{
    buffer_t *b;
    double d = value_to_dbl(i);
    uint64_t i64;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 8);
    i64 = htobe64(*((uint64_t*)&d));
    *((int64_t*)WRITE_PTR(b)) = i64;
    b->write_pos += 8;

    return self;
}

VALUE
rb_byte_buffer_append_float(VALUE self, VALUE i)
{
    buffer_t *b;
    float f = ((float)value_to_dbl(i));
    uint32_t i32;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, 4);
    i32 = htobe32(*((uint32_t*)&f));
    *((int32_t*)WRITE_PTR(b)) = i32;
    b->write_pos += 4;

    return self;
}

VALUE
rb_byte_buffer_append_byte_array(VALUE self, VALUE maybe_ary)
{
    VALUE ary = rb_check_array_type(maybe_ary);
    VALUE *ary_ptr;
    size_t len;
    buffer_t *b;

    if (NIL_P(ary))
        rb_raise(rb_eTypeError, "expected Array, got %s", rb_obj_classname(maybe_ary));

    len = RARRAY_LEN(ary);
    ary_ptr = RARRAY_PTR(ary);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_WRITE_CAPACITY(b, len);
    for (size_t i = 0; i < RARRAY_LEN(ary); ++i) {
        VALUE n = ary_ptr[i];
        int32_t i32 = value_to_int32(n);
        int8_t i8 = (int8_t)i32;

        if (i32 > 0xFF || -i32 > 0x80)
            rb_raise(rb_eRangeError, "Number %d doesn't fit into byte", i32);

        ((int8_t*)WRITE_PTR(b))[i] = i8;
    }

    b->write_pos += len;

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
rb_byte_buffer_read_long(int argc, VALUE *argv, VALUE self)
{
    VALUE f_signed;
    buffer_t *b;
    uint64_t i64;

    rb_scan_args(argc, argv, "01", &f_signed);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 8);
    i64 = be64toh(*((uint64_t*)READ_PTR(b)));
    b->read_pos += 8;

    if (RTEST(f_signed))
        return LONG2NUM((int64_t)i64);
    else
        return ULONG2NUM(i64);
}

VALUE
rb_byte_buffer_read_int(int argc, VALUE *argv, VALUE self)
{
    VALUE f_signed;
    buffer_t *b;
    uint32_t i32;

    rb_scan_args(argc, argv, "01", &f_signed);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 4);
    i32 = be32toh(*((uint32_t*)READ_PTR(b)));
    b->read_pos += 4;

    if (RTEST(f_signed))
        return INT2NUM((int32_t)i32);
    else
        return UINT2NUM(i32);
}

VALUE
rb_byte_buffer_read_short(int argc, VALUE *argv, VALUE self)
{
    VALUE f_signed;
    buffer_t *b;
    uint16_t i16;

    rb_scan_args(argc, argv, "01", &f_signed);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 2);
    i16 = be16toh(*((uint16_t*)READ_PTR(b)));
    b->read_pos += 2;

    if (RTEST(f_signed))
        return INT2NUM((int16_t)i16);
    else
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
rb_byte_buffer_read_double(VALUE self)
{
    buffer_t *b;
    uint64_t i64;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 8);
    i64 = be64toh(*((uint64_t*)READ_PTR(b)));
    b->read_pos += 8;

    return DBL2NUM(*((double*)&i64));
}

VALUE
rb_byte_buffer_read_float(VALUE self)
{
    buffer_t *b;
    uint32_t i32;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, 4);
    i32 = be32toh(*((uint32_t*)READ_PTR(b)));
    b->read_pos += 4;

    return DBL2NUM(((double)*((float*)&i32)));
}

VALUE
rb_byte_buffer_read_byte_array(int argc, VALUE *argv, VALUE self)
{
    buffer_t *b;
    VALUE n;
    VALUE f_signed;
    VALUE ary;
    long len;
    int b_signed;

    rb_scan_args(argc, argv, "11", &n, &f_signed);

    Check_Type(n, T_FIXNUM);
    len = FIX2LONG(n);
    if (len < 0) rb_raise(rb_eRangeError, "Cannot read a negative number of bytes");
    b_signed = RTEST(f_signed);

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    ENSURE_READ_CAPACITY(b, len);

    ary = rb_ary_new();
    for (int i=0; i<len; ++i) {
        uint8_t i8 = *((uint8_t*)READ_PTR(b));
        b->read_pos += 1;

        if (b_signed)
            rb_ary_push(ary, INT2NUM((int8_t)i8));
        else
            rb_ary_push(ary, UINT2NUM(i8));
    }

    return ary;
}

VALUE
rb_byte_buffer_index(int argc, VALUE *argv, VALUE self)
{
    VALUE substr;
    VALUE voffset;
    size_t offset;
    buffer_t *b;

    rb_scan_args(argc, argv, "11", &substr, &voffset);
    Check_Type(substr, T_STRING);
    if (!NIL_P(voffset)) {
        long l = NUM2LONG(voffset);
        if (l < 0) rb_raise(rb_eRangeError, "offset can't be negative");
        offset = l;
    } else
        offset = 0;

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    if (offset >= READ_SIZE(b) || offset + RSTRING_LEN(substr) > READ_SIZE(b))
        return Qnil;
    else {
        char* pos = memmem(READ_PTR(b) + offset, READ_SIZE(b), RSTRING_PTR(substr), RSTRING_LEN(substr));
        if (pos)
            return UINT2NUM(pos - READ_PTR(b));
        else
            return Qnil;
    }
}

VALUE
rb_byte_buffer_update(VALUE self, VALUE location, VALUE bytes)
{
    long offset;
    long copy_bytes_len;
    buffer_t *b;

    Check_Type(location, T_FIXNUM);
    Check_Type(bytes, T_STRING);
    offset = NUM2LONG(location);
    if (offset < 0) rb_raise(rb_eRangeError, "location can't be negative");

    TypedData_Get_Struct(self, buffer_t, &buffer_data_type, b);
    if (offset >= READ_SIZE(b))
        return self;

    copy_bytes_len = RSTRING_LEN(bytes);
    if (offset + copy_bytes_len > READ_SIZE(b))
        copy_bytes_len = READ_SIZE(b) - offset;
    memcpy(READ_PTR(b) + offset, RSTRING_PTR(bytes), copy_bytes_len);

    return self;
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

int32_t
value_to_int32(VALUE x)
{
    if (FIXNUM_P(x)) {
        int64_t i64 = FIX2LONG(x);

        if (i64 > 0xFFFFFFFF || -i64 > 0x80000000)
            rb_raise(rb_eRangeError, "Number %lld is too big", i64);

        return (int32_t)i64;
    } else if (TYPE(x) == T_BIGNUM)
        rb_raise(rb_eRangeError, "expected `interger', got `bignum'");
    else
        rb_raise(rb_eTypeError, "expected `interger', got %s ", rb_obj_classname(x));

    return 0;
}

int64_t
value_to_int64(VALUE x)
{
    if (FIXNUM_P(x))
        return FIX2LONG(x);
    else if (TYPE(x) == T_BIGNUM && RBIGNUM_SIGN(x))
        return (int64_t)rb_big2ull(x);
    else if (TYPE(x) == T_BIGNUM && !RBIGNUM_SIGN(x))
        return rb_big2ll(x);
    else
        rb_raise(rb_eTypeError, "expected `interger' or `bignum', got %s ", rb_obj_classname(x));

    return 0;
}

double
value_to_dbl(VALUE x)
{
    if (FIXNUM_P(x))
        return ((double)FIX2LONG(x));
    else if (TYPE(x) == T_FLOAT)
        return RFLOAT_VALUE(x);
    else if (TYPE(x) == T_BIGNUM)
        return rb_big2dbl(x);
    else
        rb_raise(rb_eTypeError, "expected a numeric type, got %s ", rb_obj_classname(x));

    return 0.0;
}

void
grow_buffer(buffer_t* buffer_ptr, size_t len)
{
    size_t new_size = buffer_ptr->write_pos - buffer_ptr->read_pos + len;

    if (new_size <= buffer_ptr->size) {
        memmove(buffer_ptr->b_ptr, READ_PTR(buffer_ptr), READ_SIZE(buffer_ptr));
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
