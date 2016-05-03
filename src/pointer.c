/* -*- C -*-
 * $Id$
 */

#include <ctype.h>
#include "fiddle.h"

struct RClass *cPointer;

extern struct RClass *cFiddle;
extern struct RClass *cFiddleError;

typedef void (*freefunc_t)(void*);

struct ptr_data {
    void *ptr;
    long size;
    freefunc_t free;
};

#define RPTR_DATA(obj) ((struct ptr_data *)(DATA_PTR(obj)))

static inline freefunc_t
get_freefunc(mrb_value func)
{
    if (mrb_nil_p(func)) {
    	return NULL;
    }
    return (freefunc_t)mrb_cptr(func);
}

static void
fiddle_ptr_free(mrb_state *mrb, void *ptr)
{
    struct ptr_data *data = ptr;
    if (data->ptr) {
    	if (data->free) {
    	    (*(data->free))(data->ptr);
    	}
    }
    mrb_free(mrb, ptr);
}

static const struct mrb_data_type fiddle_ptr_data_type = {
    "fiddle/pointer",
    fiddle_ptr_free
};

static mrb_value
mrb_fiddle_ptr_new2(mrb_state *mrb, struct RClass *klass, void *ptr, long size, freefunc_t func)
{
    struct ptr_data *data;
    struct RData *rdata;
    mrb_value val;

    Data_Make_Struct(mrb, klass, struct ptr_data, &fiddle_ptr_data_type, data, rdata);

    val = mrb_obj_value(rdata);
    data->ptr = ptr;
    data->free = func;
    data->size = size;

    return val;
}

static mrb_value
mrb_fiddle_ptr_new(mrb_state *mrb, void *ptr, long size, freefunc_t func)
{
    return mrb_fiddle_ptr_new2(mrb, cPointer, ptr, size, func);
}

static mrb_value
mrb_fiddle_ptr_malloc(mrb_state *mrb, long size, freefunc_t func)
{
    void *ptr;

    ptr = mrb_malloc(mrb, (size_t)size);
    memset(ptr,0,(size_t)size);
    return mrb_fiddle_ptr_new(mrb, ptr, size, func);
}

static void *
mrb_fiddle_ptr2cptr(mrb_state *mrb, mrb_value val)
{
    struct ptr_data *data;
    void *ptr;

    if (mrb_obj_is_kind_of(mrb, val, cPointer)) {
    	Data_Get_Struct(mrb, val, &fiddle_ptr_data_type, data);
    	ptr = data->ptr;
    }
    else if (mrb_nil_p(val)) {
    	ptr = NULL;
    }
    else{
    	mrb_raise(mrb, E_TYPE_ERROR, "Fiddle::Pointer was expected");
    }

    return ptr;
}

/*
 * call-seq:
 *    Fiddle::Pointer.new(address)      => fiddle_cptr
 *    new(address, size)		=> fiddle_cptr
 *    new(address, size, freefunc)	=> fiddle_cptr
 *
 * Create a new pointer to +address+ with an optional +size+ and +freefunc+.
 *
 * +freefunc+ will be called when the instance is garbage collected.
 */
static mrb_value
mrb_fiddle_ptr_initialize(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    mrb_value ptr, sym;
    mrb_int size = 0;
    mrb_int argc;

    data = (struct ptr_data *)DATA_PTR(self);
    if (data) {
        fiddle_ptr_free(mrb, data);
    }
    DATA_TYPE(self) = &fiddle_ptr_data_type;
    DATA_PTR(self) = NULL;

    data = mrb_malloc(mrb, sizeof(struct ptr_data));
    data->ptr = 0;
    data->size = 0;
    data->free = 0;
    DATA_PTR(self) = data;

    ptr = sym = mrb_nil_value();

    argc = mrb_get_args(mrb, "|oio", &ptr, &size, &sym);
    if(argc >= 1) {
        if (mrb_fixnum_p(ptr)) {
            data->ptr = (void *)mrb_fixnum(ptr);
        } else if (mrb_cptr_p(ptr)){
            data->ptr = mrb_cptr(ptr);
        } else {
            mrb_raisef(mrb, E_ARGUMENT_ERROR, "The argument must be Fixnum or C Pointer. type(%S)", mrb_fixnum_value(mrb_type(ptr)));
        }

        if (argc >= 2) data->size = (long)size;
        if (argc >= 3) data->free = get_freefunc(sym);
    }

    return self;
}

/*
 * call-seq:
 *
 *    Fiddle::Pointer.malloc(size, freefunc = nil)  => fiddle pointer instance
 *
 * Allocate +size+ bytes of memory and associate it with an optional
 * +freefunc+ that will be called when the pointer is garbage collected.
 *
 * +freefunc+ must be an address pointing to a function or an instance of
 * Fiddle::Function
 */
static mrb_value
mrb_fiddle_ptr_s_malloc(mrb_state *mrb, mrb_value klass)
{
    mrb_value obj, sym;
    mrb_int size = 0;
    long s;
    freefunc_t f = NULL;

    mrb_get_args(mrb, "i|o", &size, &sym);

	s = size;
	f = get_freefunc(sym);

    obj = mrb_fiddle_ptr_malloc(mrb, s,f);

    return obj;
}

/*
 * call-seq: to_i
 *
 * Returns the integer memory location of this pointer.
 */
static mrb_value
mrb_fiddle_ptr_to_i(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    return mrb_fixnum_value((mrb_int)data->ptr);
}

/*
 * call-seq: to_value
 *
 * Cast this pointer to a ruby object.
 */
static mrb_value
mrb_fiddle_ptr_to_value(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    return mrb_cptr_value(mrb, data->ptr);
}

void *
mrb_fiddle_ptr_to_cptr(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    return data->ptr;
}

/*
 * call-seq: ptr
 *
 * Returns a new Fiddle::Pointer instance that is a dereferenced pointer for
 * this pointer.
 *
 * Analogous to the star operator in C.
 */
static mrb_value
mrb_fiddle_ptr_ptr(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    return mrb_fiddle_ptr_new(mrb, *((void**)(data->ptr)),0,0);
}

/*
 * call-seq: ref
 *
 * Returns a new Fiddle::Pointer instance that is a reference pointer for this
 * pointer.
 *
 * Analogous to the ampersand operator in C.
 */
static mrb_value
mrb_fiddle_ptr_ref(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    return mrb_fiddle_ptr_new(mrb, &(data->ptr),0,0);
}

/*
 * call-seq: null?
 *
 * Returns +true+ if this is a null pointer.
 */
static mrb_value
mrb_fiddle_ptr_null_p(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    return data->ptr ? mrb_false_value() : mrb_true_value();
}

/*
 * call-seq: free=(function)
 *
 * Set the free function for this pointer to +function+ in the given
 * Fiddle::Function.
 */
static mrb_value
mrb_fiddle_ptr_free_set(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    mrb_value val;

    mrb_get_args(mrb, "o", &val);

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    data->free = get_freefunc(val);

    return mrb_nil_value();
}

extern mrb_value
mrb_fiddle_new_function(mrb_state *mrb, mrb_value ptr, mrb_value args, mrb_int ret_type);
/*
 * call-seq: free => Fiddle::Function
 *
 * Get the free function for this pointer.
 *
 * Returns a new instance of Fiddle::Function.
 *
 * See Fiddle::Function.new
 */
static mrb_value
mrb_fiddle_ptr_free_get(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *pdata;
    mrb_value address;
    mrb_value arg_types;
    mrb_int ret_type;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, pdata);

    if (!pdata->free)
        return mrb_nil_value();

    address = mrb_cptr_value(mrb, pdata->free);
    ret_type = TYPE_VOID;
    arg_types = mrb_ary_new_capa(mrb, 1);
    mrb_ary_push(mrb, arg_types, mrb_fixnum_value(TYPE_VOIDP));

    return mrb_fiddle_new_function(mrb, address, arg_types, ret_type);
}

/*
 * call-seq:
 *
 *    ptr.to_s        => string
 *    ptr.to_s(len)   => string
 *
 * Returns the pointer contents as a string.
 *
 * When called with no arguments, this method will return the contents until
 * the first NULL byte.
 *
 * When called with +len+, a string of +len+ bytes will be returned.
 *
 * See to_str
 */
static mrb_value
mrb_fiddle_ptr_to_s(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    mrb_value val;
    mrb_int len;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    switch (mrb_get_args(mrb, "|i", &len)) {
      case 0:
        val = mrb_str_new_cstr(mrb, (char*)(data->ptr));
        break;
      case 1:
    	val = mrb_str_new(mrb, (char*)(data->ptr), len);
    	break;
      default:
    	mrb_bug(mrb, "rb_fiddle_ptr_to_s");
    }

    return val;
}

/*
 * call-seq:
 *
 *    ptr.to_str        => string
 *    ptr.to_str(len)   => string
 *
 * Returns the pointer contents as a string.
 *
 * When called with no arguments, this method will return the contents with the
 * length of this pointer's +size+.
 *
 * When called with +len+, a string of +len+ bytes will be returned.
 *
 * See to_s
 */
static mrb_value
mrb_fiddle_ptr_to_str(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    mrb_value val;
    mrb_int len;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    switch (mrb_get_args(mrb, "|i", &len)) {
      case 0:
    	val = mrb_str_new(mrb, (char*)(data->ptr),data->size);
    	break;
      case 1:
    	val = mrb_str_new(mrb, (char*)(data->ptr), len);
    	break;
      default:
    	mrb_bug(mrb, "rb_fiddle_ptr_to_str");
    }

    return val;
}

/*
 * call-seq: inspect
 *
 * Returns a string formatted with an easily readable representation of the
 * internal state of the pointer.
 */
static mrb_value
mrb_fiddle_ptr_inspect(mrb_state *mrb, mrb_value self)
{
    struct ptr_data *data;
    char buf[256];

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    sprintf(buf, "#<%s:%p ptr=%p size=%ld free=%p>", mrb_obj_classname(mrb, self),
        data, data->ptr, data->size, data->free);

    return mrb_str_new_cstr(mrb, buf);
}

/*
 *  call-seq:
 *    ptr == other    => true or false
 *    ptr.eql?(other) => true or false
 *
 * Returns true if +other+ wraps the same pointer, otherwise returns
 * false.
 */
static mrb_value
mrb_fiddle_ptr_eql(mrb_state *mrb, mrb_value self)
{
    mrb_value other;
    void *ptr1, *ptr2;

    mrb_get_args(mrb, "o", &other);

    if(!mrb_obj_is_kind_of(mrb, other, cPointer)) return mrb_false_value();

    ptr1 = mrb_fiddle_ptr2cptr(mrb, self);
    ptr2 = mrb_fiddle_ptr2cptr(mrb, other);

    return ptr1 == ptr2 ? mrb_true_value() : mrb_false_value();
}

/*
 *  call-seq:
 *    ptr <=> other   => -1, 0, 1, or nil
 *
 * Returns -1 if less than, 0 if equal to, 1 if greater than +other+.
 *
 * Returns nil if +ptr+ cannot be compared to +other+.
 */
static mrb_value
mrb_fiddle_ptr_cmp(mrb_state *mrb, mrb_value self)
{
    mrb_value other;
    void *ptr1, *ptr2;
    long diff;

    mrb_get_args(mrb, "o", &other);

    if(!mrb_obj_is_kind_of(mrb, other, cPointer)) return mrb_nil_value();

    ptr1 = mrb_fiddle_ptr2cptr(mrb, self);
    ptr2 = mrb_fiddle_ptr2cptr(mrb, other);
    diff = (long)ptr1 - (long)ptr2;
    if (!diff) return mrb_fixnum_value(0);
    return diff > 0 ? mrb_fixnum_value(1) : mrb_fixnum_value(-1);
}

/*
 * call-seq:
 *    ptr + n   => new cptr
 *
 * Returns a new pointer instance that has been advanced +n+ bytes.
 */
static mrb_value
mrb_fiddle_ptr_plus(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    mrb_int nbytes;
    long num, size;

    mrb_get_args(mrb, "i", &nbytes);

    ptr = mrb_fiddle_ptr2cptr(mrb, self);
    size = RPTR_DATA(self)->size;
    num = nbytes;
    return mrb_fiddle_ptr_new(mrb, (char *)ptr + num, size - num, 0);
}

/*
 * call-seq:
 *    ptr - n   => new cptr
 *
 * Returns a new pointer instance that has been moved back +n+ bytes.
 */
static mrb_value
mrb_fiddle_ptr_minus(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    mrb_int nbytes;
    long num, size;

    mrb_get_args(mrb, "i", &nbytes);

    ptr = mrb_fiddle_ptr2cptr(mrb, self);
    size = RPTR_DATA(self)->size;
    num = nbytes;
    return mrb_fiddle_ptr_new(mrb, (char *)ptr - num, size + num, 0);
}

/*
 *  call-seq:
 *     ptr[index]                -> an_integer
 *     ptr[start, length]        -> a_string
 *
 * Returns integer stored at _index_.
 *
 * If _start_ and _length_ are given, a string containing the bytes from
 * _start_ of _length_ will be returned.
 */
static mrb_value
mrb_fiddle_ptr_aref(mrb_state *mrb, mrb_value self)
{
    mrb_int arg0, arg1;
    mrb_value retval = mrb_nil_value();
    size_t offset, len;
    struct ptr_data *data;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    if (!data->ptr) mrb_raise(mrb, cFiddleError, "NULL pointer dereference");
    switch( mrb_get_args(mrb, "i|i", &arg0, &arg1) ){
      case 1:
    	offset = arg0;
    	retval = mrb_fixnum_value(*((char *)data->ptr + offset));
    	break;
      case 2:
    	offset = arg0;
    	len    = arg1;
    	retval = mrb_str_new(mrb, (char *)data->ptr + offset, len);
    	break;
      default:
    	mrb_bug(mrb, "rb_fiddle_ptr_aref()");
    }
    return retval;
}

/*
 *  call-seq:
 *     ptr[index]         = int                    ->  int
 *     ptr[start, length] = string or cptr or addr ->  string or dl_cptr or addr
 *
 * Set the value at +index+ to +int+.
 *
 * Or, set the memory at +start+ until +length+ with the contents of +string+,
 * the memory from +dl_cptr+, or the memory pointed at by the memory address
 * +addr+.
 */
static mrb_value
mrb_fiddle_ptr_aset(mrb_state *mrb, mrb_value self)
{
    mrb_int arg0, arg1;
    mrb_value arg2;
    mrb_value retval = mrb_nil_value();
    size_t offset, len;
    void *mem;
    struct ptr_data *data;

    Data_Get_Struct(mrb, self, &fiddle_ptr_data_type, data);
    if (!data->ptr) mrb_raise(mrb, cFiddleError, "NULL pointer dereference");
    switch(mrb_get_args(mrb, "ii|o", &arg0, &arg1, &arg2) ){
      case 2:
    	offset = arg0;
    	((char*)data->ptr)[offset] = arg1;
    	retval = mrb_fixnum_value(arg1);
    	break;
      case 3:
    	offset = arg0;
    	len    = arg1;
    	if (mrb_string_p(arg2)) {
    	    mem = mrb_string_value_ptr(mrb, arg2);
    	}
    	else if( mrb_obj_is_kind_of(mrb, arg2, cPointer) ){
    	    mem = mrb_fiddle_ptr2cptr(mrb, arg2);
    	}
    	else{
    	    mem = mrb_cptr(arg2);
    	}
    	memcpy((char *)data->ptr + offset, mem, len);
    	retval = arg2;
    	break;
      default:
	     mrb_bug(mrb, "rb_fiddle_ptr_aset()");
    }
    return retval;
}

/*
 * call-seq: size=(size)
 *
 * Set the size of this pointer to +size+
 */
static mrb_value
mrb_fiddle_ptr_size_set(mrb_state *mrb, mrb_value self)
{
    mrb_int size;
    mrb_get_args(mrb, "i", &size);
    RPTR_DATA(self)->size = size;
    return mrb_fixnum_value(size);
}

/*
 * call-seq: size
 *
 * Get the size of this pointer.
 */
static mrb_value
mrb_fiddle_ptr_size_get(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value(RPTR_DATA(self)->size);
}

/*
 * call-seq:
 *    Fiddle::Pointer[val]         => cptr
 *    to_ptr(val)  => cptr
 *
 * Get the underlying pointer for ruby object +val+ and return it as a
 * Fiddle::Pointer object.
 */
static mrb_value
mrb_fiddle_ptr_s_to_ptr(mrb_state *mrb, mrb_value self)
{
    mrb_value ptr, val, vptr;

    mrb_get_args(mrb, "o", &val);

    if (mrb_string_p(val)){
    	char *str = mrb_string_value_ptr(mrb, val);
    	ptr = mrb_fiddle_ptr_new(mrb, str, mrb_string_value_len(mrb, val), NULL);
    }
    else if (mrb_respond_to(mrb, val, mrb_intern_lit(mrb, "to_ptr"))){
        vptr = mrb_funcall(mrb, val, "to_ptr", 0, 0);
    	if (mrb_obj_is_kind_of(mrb, vptr, cPointer)){
    	    ptr = vptr;
    	}
    	else{
    	    mrb_raise(mrb, cFiddleError, "to_ptr should return a Fiddle::Pointer object");
    	}
    }
    else{
    	ptr = mrb_fiddle_ptr_new(mrb, mrb_cptr(val), 0, NULL);
    }

    return ptr;
}

void
mrb_fiddle_pointer_init(mrb_state *mrb)
{
    /* Document-class: Fiddle::Pointer
     *
     * Fiddle::Pointer is a class to handle C pointers
     *
     */
    cPointer = mrb_define_class_under(mrb, cFiddle, "Pointer", mrb->object_class);
    MRB_SET_INSTANCE_TT(cPointer, MRB_TT_DATA);
    mrb_define_class_method(mrb, cPointer, "malloc", mrb_fiddle_ptr_s_malloc, MRB_ARGS_ARG(1, 1));
    mrb_define_class_method(mrb, cPointer, "to_ptr", mrb_fiddle_ptr_s_to_ptr, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, cPointer, "[]", mrb_fiddle_ptr_s_to_ptr, MRB_ARGS_REQ(1));

    mrb_define_method(mrb, cPointer, "initialize", mrb_fiddle_ptr_initialize, MRB_ARGS_OPT(3));
    mrb_define_method(mrb, cPointer, "free=", mrb_fiddle_ptr_free_set, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cPointer, "free",  mrb_fiddle_ptr_free_get, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "to_i",  mrb_fiddle_ptr_to_i, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "to_int",  mrb_fiddle_ptr_to_i, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "to_value",  mrb_fiddle_ptr_to_value, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "ptr",   mrb_fiddle_ptr_ptr, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "+@", mrb_fiddle_ptr_ptr, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "ref",   mrb_fiddle_ptr_ref, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "-@", mrb_fiddle_ptr_ref, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "null?", mrb_fiddle_ptr_null_p, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "to_s", mrb_fiddle_ptr_to_s, MRB_ARGS_OPT(1));
    mrb_define_method(mrb, cPointer, "to_str", mrb_fiddle_ptr_to_str, MRB_ARGS_OPT(1));
    mrb_define_method(mrb, cPointer, "inspect", mrb_fiddle_ptr_inspect, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "<=>", mrb_fiddle_ptr_cmp, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cPointer, "==", mrb_fiddle_ptr_eql, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cPointer, "eql?", mrb_fiddle_ptr_eql, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cPointer, "+", mrb_fiddle_ptr_plus, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cPointer, "-", mrb_fiddle_ptr_minus, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cPointer, "[]", mrb_fiddle_ptr_aref, MRB_ARGS_ARG(1, 1));
    mrb_define_method(mrb, cPointer, "[]=", mrb_fiddle_ptr_aset, MRB_ARGS_ARG(2, 1));
    mrb_define_method(mrb, cPointer, "size", mrb_fiddle_ptr_size_get, MRB_ARGS_NONE());
    mrb_define_method(mrb, cPointer, "size=", mrb_fiddle_ptr_size_set, MRB_ARGS_REQ(1));

    /*  Document-const: NULL
     *
     * A NULL pointer
     */
    mrb_define_const(mrb, cFiddle, "NULL", mrb_fiddle_ptr_new(mrb, 0, 0, 0));
}
