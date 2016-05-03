#include "fiddle.h"
#include "conversions.h"

struct RClass *cClosure;

extern struct RClass *cFiddle;
extern struct RClass *cPointer;

typedef struct {
    mrb_state *mrb;
    void * code;
    ffi_closure *pcl;
    ffi_cif cif;
    int argc;
    ffi_type **argv;
} fiddle_closure;

typedef struct {
    mrb_state *mrb;
    mrb_value *obj;
} closure_context;

#if defined(USE_FFI_CLOSURE_ALLOC)
#elif defined(__OpenBSD__) || defined(__APPLE__) || defined(__linux__)
# define USE_FFI_CLOSURE_ALLOC 0
#elif defined(RUBY_LIBFFI_MODVERSION) && RUBY_LIBFFI_MODVERSION < 3000005 && \
	(defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_AMD64))
# define USE_FFI_CLOSURE_ALLOC 0
#else
# define USE_FFI_CLOSURE_ALLOC 1
#endif

static void
fiddle_closure_dealloc(mrb_state *mrb, void * ptr)
{
    fiddle_closure * cls = (fiddle_closure *)ptr;
#if USE_FFI_CLOSURE_ALLOC
    ffi_closure_free(cls->pcl);
#else
    munmap(cls->pcl, sizeof(*cls->pcl));
#endif
    if (cls->argv) mrb_free(mrb, cls->argv);
    mrb_free(mrb, cls);
}

static const struct mrb_data_type closure_data_type = {
    "fiddle/closure",
    fiddle_closure_dealloc,
};

void
mrb_fiddle_closure_callback(ffi_cif *cif, void *resp, void **args, void *ctx)
{
    mrb_value self      = mrb_obj_value(ctx);
    mrb_state *mrb      = ((fiddle_closure *)(DATA_PTR(self)))->mrb;
    mrb_value rbargs    = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@args"));
    mrb_value ctype     = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@ctype"));
    mrb_int   argc      = mrb_ary_len(mrb, rbargs);
    mrb_value params    = mrb_ary_new_capa(mrb, argc);
    mrb_value ret;

    int i, type;

    for (i = 0; i < argc; i++) {
        type = mrb_fixnum(mrb_ary_entry(rbargs, i));
        switch (type) {
    	  case TYPE_VOID:
    	    argc = 0;
    	    break;
    	  case TYPE_INT:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(int *)args[i]));
    	    break;
    	  case -TYPE_INT:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(unsigned int *)args[i]));
    	    break;
    	  case TYPE_VOIDP:
    	    mrb_ary_push(mrb, params,
    			mrb_funcall(mrb, mrb_obj_value(cPointer), "[]", 1,
    				   mrb_cptr_value(mrb, *(void **)args[i])));
    	    break;
    	  case TYPE_LONG:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(long *)args[i]));
    	    break;
    	  case -TYPE_LONG:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(unsigned long *)args[i]));
    	    break;
    	  case TYPE_CHAR:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(signed char *)args[i]));
    	    break;
    	  case -TYPE_CHAR:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(unsigned char *)args[i]));
    	    break;
    	  case TYPE_SHORT:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(signed short *)args[i]));
    	    break;
    	  case -TYPE_SHORT:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(unsigned short *)args[i]));
    	    break;
    	  case TYPE_DOUBLE:
    	    mrb_ary_push(mrb, params, mrb_float_value(mrb, *(double *)args[i]));
    	    break;
    	  case TYPE_FLOAT:
    	    mrb_ary_push(mrb, params, mrb_float_value(mrb, *(float *)args[i]));
    	    break;
    #if HAVE_LONG_LONG
    	  case TYPE_LONG_LONG:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(LONG_LONG *)args[i]));
    	    break;
    	  case -TYPE_LONG_LONG:
    	    mrb_ary_push(mrb, params, mrb_fixnum_value(*(unsigned LONG_LONG *)args[i]));
    	    break;
    #endif
    	  default:
    	    mrb_raisef(mrb, E_RUNTIME_ERROR, "closure args: %S", mrb_fixnum_value(type));
        }
    }

    ret = mrb_funcall_argv(mrb, self, mrb_intern_lit(mrb, "call"), argc, RARRAY_PTR(params));

    type = mrb_fixnum(ctype);
    switch (type) {
      case TYPE_VOID:
    	break;
      case TYPE_LONG:
    	*(long *)resp = (long)mrb_int(mrb, ret);
    	break;
      case -TYPE_LONG:
    	*(unsigned long *)resp = (unsigned long)mrb_int(mrb, ret);
    	break;
      case TYPE_CHAR:
      case TYPE_SHORT:
      case TYPE_INT:
    	*(ffi_sarg *)resp = (ffi_sarg)mrb_int(mrb, ret);
    	break;
      case -TYPE_CHAR:
      case -TYPE_SHORT:
      case -TYPE_INT:
    	*(ffi_arg *)resp = (ffi_arg)mrb_int(mrb, ret);
    	break;
      case TYPE_VOIDP:
    	*(void **)resp = mrb_cptr(ret);
    	break;
      case TYPE_DOUBLE:
    	*(double *)resp = (double)mrb_float(mrb_Float(mrb, ret));
    	break;
      case TYPE_FLOAT:
    	*(float *)resp = (float)mrb_float(mrb_Float(mrb, ret));
    	break;
#if HAVE_LONG_LONG
      case TYPE_LONG_LONG:
    	*(LONG_LONG *)resp = (LONG_LONG)mrb_int(mrb, ret);
    	break;
      case -TYPE_LONG_LONG:
    	*(unsigned LONG_LONG *)resp = (unsigned LONG_LONG)mrb_int(mrb, ret);
    	break;
#endif
      default:
	     mrb_raisef(mrb, E_RUNTIME_ERROR, "closure retval: %S", mrb_fixnum_value(type));
    }
}

static mrb_value
mrb_fiddle_closure_initialize(mrb_state *mrb, mrb_value self)
{
    mrb_int ret, abi;
    mrb_value args;
    fiddle_closure * cl;
    ffi_cif * cif;
    ffi_closure *pcl;
    ffi_status result;
    mrb_int i, argc;

    cl = (fiddle_closure *)DATA_PTR(self);
    if (cl) {
        fiddle_closure_dealloc(mrb, cl);
    }
    DATA_TYPE(self) = &closure_data_type;
    DATA_PTR(self) = NULL;

    cl = mrb_malloc(mrb, sizeof(fiddle_closure));
    cl->mrb = mrb;
#if USE_FFI_CLOSURE_ALLOC
    cl->pcl = ffi_closure_alloc(sizeof(ffi_closure), &cl->code);
#else
    cl->pcl = mmap(NULL, sizeof(ffi_closure), PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
    DATA_PTR(self) = cl;

    if (2 == mrb_get_args(mrb, "iA|i", &ret, &args, &abi))
    	abi = FFI_DEFAULT_ABI;

    //Check_Type(args, T_ARRAY);

    argc = mrb_ary_len(mrb, args);

    cl->argv = (ffi_type **)mrb_calloc(mrb, argc + 1, sizeof(ffi_type *));

    for (i = 0; i < argc; i++) {
        int type = mrb_fixnum(mrb_ary_entry(args, i));
        cl->argv[i] = INT2FFI_TYPE(mrb, type);
    }
    cl->argv[argc] = NULL;

    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@ctype"), mrb_fixnum_value(ret));
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@args"), args);

    cif = &cl->cif;
    pcl = cl->pcl;

    result = ffi_prep_cif(cif, abi, argc, INT2FFI_TYPE(mrb, ret), cl->argv);

    if (FFI_OK != result)
    	mrb_raisef(mrb, E_RUNTIME_ERROR, "error prepping CIF %S", mrb_fixnum_value(result));

#if USE_FFI_CLOSURE_ALLOC
    result = ffi_prep_closure_loc(pcl, cif, mrb_fiddle_closure_callback,
		(void *)RDATA(self), cl->code);
#else
    result = ffi_prep_closure(pcl, cif, mrb_fiddle_closure_callback, (void *)RDATA(self));
    cl->code = (void *)pcl;
    i = mprotect(pcl, sizeof(*pcl), PROT_READ | PROT_EXEC);
    if (i) {
        mrb_sys_fail(mrb, "mprotect");
    }
#endif

    if (FFI_OK != result)
    	mrb_raisef(mrb, E_RUNTIME_ERROR, "error prepping closure %S", mrb_fixnum_value(result));

    return self;
}

static mrb_value
mrb_fiddle_closure_to_i(mrb_state *mrb, mrb_value self)
{
    fiddle_closure * cl;
    void *code;

    Data_Get_Struct(mrb, self, &closure_data_type, cl);

    code = cl->code;

    return mrb_fixnum_value((long)code);
}

static mrb_value
mrb_fiddle_closure_to_value(mrb_state *mrb, mrb_value self)
{
    fiddle_closure * cl;

    Data_Get_Struct(mrb, self, &closure_data_type, cl);
    return mrb_cptr_value(mrb, cl->code);
}

void
mrb_fiddle_closure_init(mrb_state *mrb)
{
    /*
     * Document-class: Fiddle::Closure
     *
     * == Description
     *
     * An FFI closure wrapper, for handling callbacks.
     *
     * == Example
     *
     *   closure = Class.new(Fiddle::Closure) {
     *     def call
     *       10
     *     end
     *   }.new(Fiddle::TYPE_INT, [])
     *	    #=> #<#<Class:0x0000000150d308>:0x0000000150d240>
     *   func = Fiddle::Function.new(closure, [], Fiddle::TYPE_INT)
     *	    #=> #<Fiddle::Function:0x00000001516e58>
     *   func.call
     *	    #=> 10
     */
    cClosure = mrb_define_class_under(mrb, cFiddle, "Closure", mrb->object_class);
    MRB_SET_INSTANCE_TT(cClosure, MRB_TT_DATA);
    /*
     * Document-method: new
     *
     * call-seq: new(ret, args, abi = Fiddle::DEFAULT)
     *
     * Construct a new Closure object.
     *
     * * +ret+ is the C type to be returned
     * * +args+ is an Array of arguments, passed to the callback function
     * * +abi+ is the abi of the closure
     *
     * If there is an error in preparing the ffi_cif or ffi_prep_closure,
     * then a RuntimeError will be raised.
     */
    mrb_define_method(mrb, cClosure, "initialize", mrb_fiddle_closure_initialize, MRB_ARGS_ARG(2, 1));

    /*
     * Document-method: to_i
     *
     * Returns the memory address for this closure
     */
    mrb_define_method(mrb, cClosure, "to_i", mrb_fiddle_closure_to_i, MRB_ARGS_NONE());
    mrb_define_method(mrb, cClosure, "to_value", mrb_fiddle_closure_to_value, MRB_ARGS_NONE());
}
/* vim: set noet sw=4 sts=4 */
