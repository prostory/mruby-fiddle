#include "fiddle.h"
#include "conversions.h"

struct RClass *cFunction;
extern struct RClass *cFiddle;
extern struct RClass *cPointer;

static void
mrb_function_free(mrb_state *mrb, void *p)
{
    ffi_cif *ptr = p;
    if (ptr->arg_types) mrb_free(mrb, ptr->arg_types);
    mrb_free(mrb, ptr);
}

static const struct mrb_data_type function_data_type = {
    "fiddle/function",
    mrb_function_free,
};

static mrb_value
mrb_fiddle_func_new(mrb_state *mrb)
{
    ffi_cif * cif;
    struct RData *data;

    Data_Make_Struct(mrb, cFunction, ffi_cif, &function_data_type, cif, data);

    return mrb_obj_value(data);
}

static mrb_value
mrb_fiddle_new_function_full(mrb_state *mrb, mrb_value self, mrb_value ptr, mrb_value args,
    mrb_int ret_type, mrb_int abi, mrb_value name)
{
    ffi_cif * cif;
    ffi_type **arg_types;
    ffi_status result;
    mrb_int i, args_len;

    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@ptr"), ptr);
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@args"), args);
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@return_type"), mrb_fixnum_value(ret_type));
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@abi"), mrb_fixnum_value(abi));
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@name"), name);

    Data_Get_Struct(mrb, self, &function_data_type, cif);

    args_len = mrb_ary_len(mrb, args);

    arg_types = mrb_calloc(mrb, args_len + 1, sizeof(ffi_type *));

    for (i = 0; i < args_len; i++) {
        mrb_int type = mrb_fixnum(mrb_ary_entry(args, i));
        arg_types[i] = INT2FFI_TYPE(mrb, type);
    }
    arg_types[args_len] = NULL;

    result = ffi_prep_cif (
        cif,
        abi,
        args_len,
        INT2FFI_TYPE(mrb, ret_type),
        arg_types);

    if (result)
       mrb_raisef(mrb, E_RUNTIME_ERROR, "error creating CIF %S", mrb_fixnum_value(result));

    return self;
}

mrb_value
mrb_fiddle_new_function(mrb_state *mrb, mrb_value ptr, mrb_value args, mrb_int ret_type)
{
    mrb_value self;

    self = mrb_fiddle_func_new(mrb);
    return mrb_fiddle_new_function_full(mrb, self, ptr, args, ret_type, FFI_DEFAULT_ABI, mrb_nil_value());
}

static mrb_value
mrb_fiddle_func_initialize(mrb_state *mrb, mrb_value self)
{
    ffi_cif * cif;
    mrb_value ptr, args, name = mrb_nil_value();
    mrb_int ret_type = TYPE_VOID, abi = FFI_DEFAULT_ABI;

    mrb_get_args(mrb, "oA|iiS", &ptr, &args, &ret_type, &abi, &name);

    cif = (ffi_cif *)DATA_PTR(self);
    if (cif) {
        mrb_function_free(mrb, cif);
    }
    DATA_TYPE(self) = &function_data_type;
    DATA_PTR(self) = NULL;

    cif = mrb_malloc(mrb, sizeof(ffi_cif));
    DATA_PTR(self) = cif;

    if (mrb_respond_to(mrb, ptr, mrb_intern_lit(mrb, "to_value"))) {
        ptr = mrb_funcall(mrb, ptr, "to_value", 0, NULL);
    }

    return mrb_fiddle_new_function_full(mrb, self, ptr, args, ret_type, abi, name);
}



static mrb_value
mrb_fiddle_func_call(mrb_state *mrb, mrb_value self)
{
    ffi_cif * cif;
    fiddle_generic retval;
    fiddle_generic *generic_args;
    void **values;
    mrb_value *argv, cfunc, types;
    mrb_int i, argc, args_len;

    mrb_get_args(mrb, "*", &argv, &argc);

    cfunc    = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@ptr"));
    types    = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@args"));

    args_len = mrb_ary_len(mrb, types);

    if(argc != args_len) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%S for %S)",
		      mrb_fixnum_value(argc), mrb_fixnum_value(args_len));
    }

    Data_Get_Struct(mrb, self, &function_data_type, cif);

    generic_args = mrb_malloc(mrb, (size_t)(argc + 1) * sizeof(void *) + (size_t)argc * sizeof(fiddle_generic));
    values = (void **)((char *)generic_args + (size_t)argc * sizeof(fiddle_generic));

    for (i = 0; i < argc; i++) {
    	mrb_value type = mrb_ary_entry(types, i);
    	mrb_value src = argv[i];

    	if(mrb_fixnum(type) == TYPE_VOIDP) {
    	    if(mrb_nil_p(src)) {
                src = mrb_cptr_value(mrb, NULL);
    	    } else if(!mrb_obj_is_instance_of(mrb, src, cPointer)) {
                src = mrb_funcall(mrb, mrb_obj_value(cPointer), "[]", 1, src);
    	    }
    	}

    	VALUE2GENERIC(mrb, mrb_fixnum(type), src, &generic_args[i]);
    	values[i] = (void *)&generic_args[i];
    }
    values[argc] = NULL;

    ffi_call(cif, mrb_cptr(cfunc), &retval, values);

    mrb_free(mrb, generic_args);

    mrb_funcall(mrb, mrb_obj_value(cFiddle), "last_error=", 1, mrb_fixnum_value(errno));
#if defined(_WIN32)
    mrb_funcall(mrb, mrb_obj_value(cFiddle), "win32_last_error=", 1, mrb_fixnum_value(errno));
#endif

    return GENERIC2VALUE(mrb, mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@return_type")), retval);
}

void
mrb_fiddle_function_init(mrb_state *mrb)
{
    /*
     * Document-class: Fiddle::Function
     *
     * == Description
     *
     * A representation of a C function
     *
     * == Examples
     *
     * === 'strcpy'
     *
     *   @libc = Fiddle.dlopen "/lib/libc.so.6"
     *	    #=> #<Fiddle::Handle:0x00000001d7a8d8>
     *   f = Fiddle::Function.new(
     *     @libc['strcpy'],
     *     [Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP],
     *     Fiddle::TYPE_VOIDP)
     *	    #=> #<Fiddle::Function:0x00000001d8ee00>
     *   buff = "000"
     *	    #=> "000"
     *   str = f.call(buff, "123")
     *	    #=> #<Fiddle::Pointer:0x00000001d0c380 ptr=0x000000018a21b8 size=0 free=0x00000000000000>
     *   str.to_s
     *   => "123"
     *
     * === ABI check
     *
     *   @libc = Fiddle.dlopen "/lib/libc.so.6"
     *	    #=> #<Fiddle::Handle:0x00000001d7a8d8>
     *   f = Fiddle::Function.new(@libc['strcpy'], [TYPE_VOIDP, TYPE_VOIDP], TYPE_VOIDP)
     *	    #=> #<Fiddle::Function:0x00000001d8ee00>
     *   f.abi == Fiddle::Function::DEFAULT
     *	    #=> true
     */
    cFunction = mrb_define_class_under(mrb, cFiddle, "Function", mrb->object_class);
    MRB_SET_INSTANCE_TT(cFunction, MRB_TT_DATA);

    /*
     * Document-const: DEFAULT
     *
     * Default ABI
     *
     */
    mrb_define_const(mrb, cFunction, "DEFAULT", mrb_fixnum_value(FFI_DEFAULT_ABI));

#ifdef HAVE_CONST_FFI_STDCALL
    /*
     * Document-const: STDCALL
     *
     * FFI implementation of WIN32 stdcall convention
     *
     */
    mrb_define_const(mrb, cFunction, "STDCALL", mrb_fixnum_value(FFI_STDCALL));
#endif

    /*
     * Document-method: new
     * call-seq: new(ptr, args, ret_type, abi = DEFAULT)
     *
     * Constructs a Function object.
     * * +ptr+ is a referenced function, of a Fiddle::Handle
     * * +args+ is an Array of arguments, passed to the +ptr+ function
     * * +ret_type+ is the return type of the function
     * * +abi+ is the ABI of the function
     *
     */
    mrb_define_method(mrb, cFunction, "initialize", mrb_fiddle_func_initialize, MRB_ARGS_ARG(2, 3));

    /*
     * Document-method: call
     *
     * Calls the constructed Function, with +args+
     *
     * For an example see Fiddle::Function
     *
     */
    mrb_define_method(mrb, cFunction, "call", mrb_fiddle_func_call, MRB_ARGS_ANY());
}
/* vim: set noet sws=4 sw=4: */
