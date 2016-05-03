#ifndef FIDDLE_CONVERSIONS_H
#define FIDDLE_CONVERSIONS_H

#include "fiddle.h"

typedef union
{
    ffi_arg  fffi_arg;     /* rvalue smaller than unsigned long */
    ffi_sarg fffi_sarg;    /* rvalue smaller than signed long */
    unsigned char uchar;   /* ffi_type_uchar */
    signed char   schar;   /* ffi_type_schar */
    unsigned short ushort; /* ffi_type_sshort */
    signed short sshort;   /* ffi_type_ushort */
    unsigned int uint;     /* ffi_type_uint */
    signed int sint;       /* ffi_type_sint */
    unsigned long ulong;   /* ffi_type_ulong */
    signed long slong;     /* ffi_type_slong */
    float ffloat;          /* ffi_type_float */
    double ddouble;        /* ffi_type_double */
#if HAVE_LONG_LONG
    unsigned LONG_LONG ulong_long; /* ffi_type_ulong_long */
    signed LONG_LONG slong_long; /* ffi_type_ulong_long */
#endif
    void * pointer;        /* ffi_type_pointer */
} fiddle_generic;

ffi_type * int_to_ffi_type(mrb_state *mrb, int type);
void value_to_generic(mrb_state *mrb, int type, mrb_value src, fiddle_generic * dst);
mrb_value generic_to_value(mrb_state *mrb, mrb_value rettype, fiddle_generic retval);

#define VALUE2GENERIC(_mrb, _type, _src, _dst) value_to_generic((_mrb), (_type), (_src), (_dst))
#define INT2FFI_TYPE(_mrb, _type) int_to_ffi_type((_mrb), (_type))
#define GENERIC2VALUE(_mrb, _type, _retval) generic_to_value((_mrb), (_type), (_retval))

#endif
