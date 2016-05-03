#include "fiddle.h"
#include "conversions.h"

extern struct RClass *cPointer;

ffi_type *
int_to_ffi_type(mrb_state *mrb, int type)
{
    int signed_p = 1;

    if (type < 0) {
    	type = -1 * type;
    	signed_p = 0;
    }

#define rb_ffi_type_of(t) (signed_p ? &ffi_type_s##t : &ffi_type_u##t)

    switch (type) {
      case TYPE_VOID:
    	return &ffi_type_void;
      case TYPE_VOIDP:
    	return &ffi_type_pointer;
      case TYPE_CHAR:
    	return rb_ffi_type_of(char);
      case TYPE_SHORT:
    	return rb_ffi_type_of(short);
      case TYPE_INT:
    	return rb_ffi_type_of(int);
      case TYPE_LONG:
    	return rb_ffi_type_of(long);
#if HAVE_LONG_LONG
      case TYPE_LONG_LONG:
    	return rb_ffi_type_of(long_long);
#endif
      case TYPE_FLOAT:
    	return &ffi_type_float;
      case TYPE_DOUBLE:
    	return &ffi_type_double;
      default:
    	mrb_raisef(mrb, E_RUNTIME_ERROR, "unknown type %S", mrb_fixnum_value(type));
    }
    return &ffi_type_pointer;
}

extern void *
mrb_fiddle_ptr_to_cptr(mrb_state *mrb, mrb_value self);

void
value_to_generic(mrb_state *mrb, int type, mrb_value src, fiddle_generic * dst)
{
    switch (type) {
      case TYPE_VOID:
    	break;
      case TYPE_VOIDP:
    	dst->pointer = mrb_fiddle_ptr_to_cptr(mrb, src);
    	break;
      case TYPE_CHAR:
    	dst->schar = (signed char)mrb_int(mrb, src);
    	break;
      case -TYPE_CHAR:
    	dst->uchar = (unsigned char)mrb_int(mrb, src);
    	break;
      case TYPE_SHORT:
    	dst->sshort = (unsigned short)mrb_int(mrb, src);
    	break;
      case -TYPE_SHORT:
    	dst->sshort = (signed short)mrb_int(mrb, src);
    	break;
      case TYPE_INT:
    	dst->sint = (signed int)mrb_int(mrb, src);
    	break;
      case -TYPE_INT:
    	dst->uint = (unsigned int)mrb_int(mrb, src);
    	break;
      case TYPE_LONG:
    	dst->slong = (signed long)mrb_int(mrb, src);
    	break;
      case -TYPE_LONG:
    	dst->ulong = (unsigned long)mrb_int(mrb, src);
    	break;
#if HAVE_LONG_LONG
      case TYPE_LONG_LONG:
    	dst->slong_long = (signed LONG_LONG)mrb_int(mrb, src);
    	break;
      case -TYPE_LONG_LONG:
    	dst->ulong_long = (unsigned LONG_LONG)mrb_int(mrb, src);
    	break;
#endif
      case TYPE_FLOAT:
    	dst->ffloat = (float)mrb_float(mrb_Float(mrb, src));
    	break;
      case TYPE_DOUBLE:
    	dst->ddouble = (double)mrb_float(mrb_Float(mrb, src));
    	break;
      default:
	     mrb_raisef(mrb, E_RUNTIME_ERROR, "unknown type %S", mrb_fixnum_value(type));
    }
}

mrb_value
generic_to_value(mrb_state *mrb, mrb_value rettype, fiddle_generic retval)
{
    int type = mrb_int(mrb, rettype);

    switch (type) {
      case TYPE_VOID:
    	return mrb_nil_value();
      case TYPE_VOIDP:
        return mrb_funcall(mrb, mrb_obj_value(cPointer), "[]",
            1, mrb_cptr_value(mrb, (void *)retval.pointer));
      case TYPE_CHAR:
    	return mrb_fixnum_value((signed char)retval.fffi_sarg);
      case -TYPE_CHAR:
    	return mrb_fixnum_value((unsigned char)retval.fffi_arg);
      case TYPE_SHORT:
    	return mrb_fixnum_value((signed short)retval.fffi_sarg);
      case -TYPE_SHORT:
    	return mrb_fixnum_value((unsigned short)retval.fffi_arg);
      case TYPE_INT:
    	return mrb_fixnum_value((signed int)retval.fffi_sarg);
      case -TYPE_INT:
    	return mrb_fixnum_value((unsigned int)retval.fffi_arg);
      case TYPE_LONG:
    	return mrb_fixnum_value(retval.slong);
      case -TYPE_LONG:
    	return mrb_fixnum_value(retval.ulong);
#if HAVE_LONG_LONG
      case TYPE_LONG_LONG:
    	return mrb_fixnum_value(retval.slong_long);
      case -TYPE_LONG_LONG:
    	return mrb_fixnum_value(retval.ulong_long);
#endif
      case TYPE_FLOAT:
    	return mrb_float_value(mrb, retval.ffloat);
      case TYPE_DOUBLE:
    	return mrb_float_value(mrb, retval.ddouble);
      default:
	     mrb_raisef(mrb, E_RUNTIME_ERROR, "unknown type %S", mrb_fixnum_value(type));
    }

    return mrb_nil_value();
}

/* vim: set noet sw=4 sts=4 */
