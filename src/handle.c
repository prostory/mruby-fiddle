#include "fiddle.h"

struct RClass *cHandle;

extern struct RClass *cFiddle;
extern struct RClass *cFiddleError;

struct dl_handle {
    void *ptr;
    int  open;
    int  enable_close;
};

#ifdef _WIN32
# ifndef _WIN32_WCE
static void *
w32_coredll(void)
{
    MEMORY_BASIC_INFORMATION m;
    memset(&m, 0, sizeof(m));
    if( !VirtualQuery(_errno, &m, sizeof(m)) ) return NULL;
    return m.AllocationBase;
}
# endif

static int
w32_dlclose(void *ptr)
{
# ifndef _WIN32_WCE
    if( ptr == w32_coredll() ) return 0;
# endif
    if( FreeLibrary((HMODULE)ptr) ) return 0;
    return errno = rb_w32_map_errno(GetLastError());
}
#define dlclose(ptr) w32_dlclose(ptr)
#endif

static void
fiddle_handle_free(mrb_state *mrb, void *ptr)
{
    struct dl_handle *fiddle_handle = ptr;
    if( fiddle_handle->ptr && fiddle_handle->open && fiddle_handle->enable_close ){
    	dlclose(fiddle_handle->ptr);
    }
    mrb_free(mrb, ptr);
}

static const struct mrb_data_type fiddle_handle_data_type = {
    "fiddle/handle",
    fiddle_handle_free
};

/*
 * call-seq: close
 *
 * Close this handle.
 *
 * Calling close more than once will raise a Fiddle::DLError exception.
 */
static mrb_value
mrb_fiddle_handle_close(mrb_state *mrb, mrb_value self)
{
    struct dl_handle *fiddle_handle;

    Data_Get_Struct(mrb, self, &fiddle_handle_data_type, fiddle_handle);
    if(fiddle_handle->open) {
    	int ret = dlclose(fiddle_handle->ptr);
    	fiddle_handle->open = 0;

    	/* Check dlclose for successful return value */
    	if(ret) {
#if defined(HAVE_DLERROR)
    	    mrb_raisef(mrb, cFiddleError, "%S", mrb_str_new_cstr(mrb, dlerror()));
#else
    	    mrb_raise(mrb, cFiddleError, "could not close handle");
#endif
    	}
    	return mrb_fixnum_value(ret);
    }
    mrb_raise(mrb, cFiddleError, "dlclose() called too many times");

    return mrb_nil_value();
}

static mrb_value
mrb_fiddle_handle_new(mrb_state *mrb, struct RClass *klass)
{
    mrb_value obj;
    struct dl_handle *fiddle_handle;
    struct RData *data;

    Data_Make_Struct(mrb, klass, struct dl_handle, &fiddle_handle_data_type, fiddle_handle, data);

    obj = mrb_obj_value(data);
    fiddle_handle->ptr  = 0;
    fiddle_handle->open = 0;
    fiddle_handle->enable_close = 0;

    return obj;
}

static mrb_value
predefined_fiddle_handle(mrb_state *mrb, void *handle)
{
    mrb_value obj = mrb_fiddle_handle_new(mrb, cHandle);
    struct dl_handle *fiddle_handle = DATA_PTR(obj);

    fiddle_handle->ptr = handle;
    fiddle_handle->open = 1;

    return obj;
}

/*
 * call-seq:
 *    new(library = nil, flags = Fiddle::RTLD_LAZY | Fiddle::RTLD_GLOBAL)
 *
 * Create a new handler that opens +library+ with +flags+.
 *
 * If no +library+ is specified or +nil+ is given, DEFAULT is used, which is
 * the equivalent to RTLD_DEFAULT. See <code>man 3 dlopen</code> for more.
 *
 *	lib = Fiddle::Handle.new
 *
 * The default is dependent on OS, and provide a handle for all libraries
 * already loaded. For example, in most cases you can use this to access +libc+
 * functions, or ruby functions like +rb_str_new+.
 */
static mrb_value
mrb_fiddle_handle_initialize(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    struct dl_handle *fiddle_handle;
    mrb_value lib = mrb_nil_value();
    mrb_int flag = RTLD_LAZY | RTLD_GLOBAL;
    char  *clib;
    int   cflag;
    const char *err;

    fiddle_handle = (struct dl_handle *)DATA_PTR(self);
    if (fiddle_handle) {
        fiddle_handle_free(mrb, fiddle_handle);
    }

    DATA_TYPE(self) = &fiddle_handle_data_type;
    DATA_PTR(self) = NULL;

    fiddle_handle = mrb_malloc(mrb, sizeof(struct dl_handle));
    fiddle_handle->ptr  = 0;
    fiddle_handle->open = 0;
    fiddle_handle->enable_close = 0;
    DATA_PTR(self) = fiddle_handle;;

    switch( mrb_get_args(mrb, "|Si", &lib, &flag) ){
      case 0:
    	clib = NULL;
    	cflag = RTLD_LAZY | RTLD_GLOBAL;
    	break;
      case 1:
    	clib = mrb_nil_p(lib) ? NULL : mrb_string_value_cstr(mrb, &lib);
    	cflag = RTLD_LAZY | RTLD_GLOBAL;
    	break;
      case 2:
    	clib = mrb_nil_p(lib) ? NULL : mrb_string_value_cstr(mrb, &lib);
    	cflag = flag;
    	break;
      default:
    	mrb_bug(mrb, "rb_fiddle_handle_new");
    }

#if defined(_WIN32)
    if( !clib ){
    	HANDLE rb_libruby_handle(void);
    	ptr = rb_libruby_handle();
    }
    else if( STRCASECMP(clib, "libc") == 0
# ifdef RUBY_COREDLL
	     || STRCASECMP(clib, RUBY_COREDLL) == 0
	     || STRCASECMP(clib, RUBY_COREDLL".dll") == 0
# endif
	){
# ifdef _WIN32_WCE
    	ptr = dlopen("coredll.dll", cflag);
# else
    	(void)cflag;
    	ptr = w32_coredll();
# endif
    }
    else
#endif
	ptr = dlopen(clib, cflag);
#if defined(HAVE_DLERROR)
    if( !ptr && (err = dlerror()) ){
    	mrb_raisef(mrb, cFiddleError, "%S", mrb_str_new_cstr(mrb, err));
    }
#else
    if( !ptr ){
    	err = dlerror();
    	mrb_raisef(mrb, cFiddleError, "%S", mrb_str_new_cstr(mrb, err));
    }
#endif
    if( fiddle_handle->ptr && fiddle_handle->open && fiddle_handle->enable_close ){
    	dlclose(fiddle_handle->ptr);
    }
    fiddle_handle->ptr = ptr;
    fiddle_handle->open = 1;
    fiddle_handle->enable_close = 0;

    return self;
}

/*
 * call-seq: enable_close
 *
 * Enable a call to dlclose() when this handle is garbage collected.
 */
static mrb_value
mrb_fiddle_handle_enable_close(mrb_state *mrb, mrb_value self)
{
    struct dl_handle *fiddle_handle;

    Data_Get_Struct(mrb, self, &fiddle_handle_data_type, fiddle_handle);
    fiddle_handle->enable_close = 1;
    return mrb_nil_value();
}

/*
 * call-seq: disable_close
 *
 * Disable a call to dlclose() when this handle is garbage collected.
 */
static mrb_value
mrb_fiddle_handle_disable_close(mrb_state *mrb, mrb_value self)
{
    struct dl_handle *fiddle_handle;

    Data_Get_Struct(mrb, self, &fiddle_handle_data_type, fiddle_handle);
    fiddle_handle->enable_close = 0;
    return mrb_nil_value();
}

/*
 * call-seq: close_enabled?
 *
 * Returns +true+ if dlclose() will be called when this handle is garbage collected.
 *
 * See man(3) dlclose() for more info.
 */
static mrb_value
mrb_fiddle_handle_close_enabled_p(mrb_state *mrb, mrb_value self)
{
    struct dl_handle *fiddle_handle;

    Data_Get_Struct(mrb, self, &fiddle_handle_data_type, fiddle_handle);

    if(fiddle_handle->enable_close) return mrb_true_value();
    return mrb_false_value();
}

/*
 * call-seq: to_i
 *
 * Returns the memory address for this handle.
 */
static mrb_value
mrb_fiddle_handle_to_i(mrb_state *mrb, mrb_value self)
{
    struct dl_handle *fiddle_handle;

    Data_Get_Struct(mrb, self, &fiddle_handle_data_type, fiddle_handle);
    return mrb_fixnum_value((mrb_int)fiddle_handle);
}

static mrb_value fiddle_handle_sym(mrb_state *mrb, void *handle, const char *symbol);

/*
 * Document-method: sym
 *
 * call-seq: sym(name)
 *
 * Get the address as an Integer for the function named +name+.
 */
static mrb_value
mrb_fiddle_handle_sym(mrb_state *mrb, mrb_value self)
{
    struct dl_handle *fiddle_handle;
    mrb_value sym;

    mrb_get_args(mrb, "S", &sym);

    Data_Get_Struct(mrb, self, &fiddle_handle_data_type, fiddle_handle);
    if( ! fiddle_handle->open ){
    	mrb_raisef(mrb, cFiddleError, "closed handle");
    }

    return fiddle_handle_sym(mrb, fiddle_handle->ptr, mrb_string_value_cstr(mrb, &sym));
}

#ifndef RTLD_NEXT
#define RTLD_NEXT NULL
#endif
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT NULL
#endif

/*
 * Document-method: sym
 *
 * call-seq: sym(name)
 *
 * Get the address as an Integer for the function named +name+.  The function
 * is searched via dlsym on RTLD_NEXT.
 *
 * See man(3) dlsym() for more info.
 */
static mrb_value
mrb_fiddle_handle_s_sym(mrb_state *mrb, mrb_value self)
{
    mrb_value sym;

    mrb_get_args(mrb, "S", &sym);

    return fiddle_handle_sym(mrb, RTLD_NEXT, mrb_string_value_cstr(mrb, &sym));
}

static mrb_value
fiddle_handle_sym(mrb_state *mrb, void *handle, const char *name)
{
#if defined(HAVE_DLERROR)
    const char *err;
# define CHECK_DLERROR if ((err = dlerror()) != 0) { func = 0; }
#else
# define CHECK_DLERROR
#endif
    void (*func)();

#ifdef HAVE_DLERROR
    dlerror();
#endif
    func = dlsym(handle, name);
    CHECK_DLERROR;
#if defined(FUNC_STDCALL)
    if( !func ){
    	int  i;
    	int  len = (int)strlen(name);
    	char *name_n;
#if defined(__CYGWIN__) || defined(_WIN32) || defined(__MINGW32__)
    	{
    	    char *name_a = (char*)mrb_malloc(mrb, len+2);
    	    strcpy(name_a, name);
    	    name_n = name_a;
    	    name_a[len]   = 'A';
    	    name_a[len+1] = '\0';
    	    func = dlsym(handle, name_a);
    	    CHECK_DLERROR;
    	    if( func ) goto found;
    	    name_n = mrb_realloc(mrb, name_a, len+6);
    	}
#else
    	name_n = (char*)mrb_malloc(mrb, len+6);
#endif
    	memcpy(name_n, name, len);
    	name_n[len++] = '@';
    	for( i = 0; i < 256; i += 4 ){
    	    sprintf(name_n + len, "%d", i);
    	    func = dlsym(handle, name_n);
    	    CHECK_DLERROR;
    	    if( func ) break;
    	}
    	if( func ) goto found;
    	name_n[len-1] = 'A';
    	name_n[len++] = '@';
    	for( i = 0; i < 256; i += 4 ){
    	    sprintf(name_n + len, "%d", i);
    	    func = dlsym(handle, name_n);
    	    CHECK_DLERROR;
    	    if( func ) break;
    	}
found:
    	mrb_free(mrb, name_n);
    }
#endif
    if( !func ){
    	mrb_raisef(mrb, cFiddleError, "unknown symbol \"%S\"", mrb_str_new_cstr(mrb, name));
    }

    return mrb_cptr_value(mrb, func);
}

void
mrb_fiddle_handle_init(mrb_state *mrb)
{
    /*
     * Document-class: Fiddle::Handle
     *
     * The Fiddle::Handle is the manner to access the dynamic library
     *
     * == Example
     *
     * === Setup
     *
     *   libc_so = "/lib64/libc.so.6"
     *   => "/lib64/libc.so.6"
     *   @handle = Fiddle::Handle.new(libc_so)
     *   => #<Fiddle::Handle:0x00000000d69ef8>
     *
     * === Setup, with flags
     *
     *   libc_so = "/lib64/libc.so.6"
     *   => "/lib64/libc.so.6"
     *   @handle = Fiddle::Handle.new(libc_so, Fiddle::RTLD_LAZY | Fiddle::RTLD_GLOBAL)
     *   => #<Fiddle::Handle:0x00000000d69ef8>
     *
     * See RTLD_LAZY and RTLD_GLOBAL
     *
     * === Addresses to symbols
     *
     *   strcpy_addr = @handle['strcpy']
     *   => 140062278451968
     *
     * or
     *
     *   strcpy_addr = @handle.sym('strcpy')
     *   => 140062278451968
     *
     */
    cHandle = mrb_define_class_under(mrb, cFiddle, "Handle", mrb->object_class);
    MRB_SET_INSTANCE_TT(cHandle, MRB_TT_DATA);

    mrb_define_class_method(mrb, cHandle, "sym", mrb_fiddle_handle_s_sym, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, cHandle, "[]", mrb_fiddle_handle_s_sym,  MRB_ARGS_REQ(1));

    /* Document-const: NEXT
     *
     * A predefined pseudo-handle of RTLD_NEXT
     *
     * Which will find the next occurrence of a function in the search order
     * after the current library.
     */
    mrb_define_const(mrb, cHandle, "NEXT", predefined_fiddle_handle(mrb, RTLD_NEXT));

    /* Document-const: DEFAULT
     *
     * A predefined pseudo-handle of RTLD_DEFAULT
     *
     * Which will find the first occurrence of the desired symbol using the
     * default library search order
     */
    mrb_define_const(mrb, cHandle, "DEFAULT", predefined_fiddle_handle(mrb, RTLD_DEFAULT));

    /* Document-const: RTLD_GLOBAL
     *
     * rtld Fiddle::Handle flag.
     *
     * The symbols defined by this library will be made available for symbol
     * resolution of subsequently loaded libraries.
     */
    mrb_define_const(mrb, cHandle, "RTLD_GLOBAL", mrb_fixnum_value(RTLD_GLOBAL));

    /* Document-const: RTLD_LAZY
     *
     * rtld Fiddle::Handle flag.
     *
     * Perform lazy binding.  Only resolve symbols as the code that references
     * them is executed.  If the  symbol is never referenced, then it is never
     * resolved.  (Lazy binding is only performed for function references;
     * references to variables are always immediately bound when the library
     * is loaded.)
     */
    mrb_define_const(mrb, cHandle, "RTLD_LAZY",   mrb_fixnum_value(RTLD_LAZY));

    /* Document-const: RTLD_NOW
     *
     * rtld Fiddle::Handle flag.
     *
     * If this value is specified or the environment variable LD_BIND_NOW is
     * set to a nonempty string, all undefined symbols in the library are
     * resolved before Fiddle.dlopen returns.  If this cannot be done an error
     * is returned.
     */
    mrb_define_const(mrb, cHandle, "RTLD_NOW",    mrb_fixnum_value(RTLD_NOW));

    mrb_define_method(mrb, cHandle, "initialize", mrb_fiddle_handle_initialize, MRB_ARGS_OPT(2));
    mrb_define_method(mrb, cHandle, "to_i", mrb_fiddle_handle_to_i, MRB_ARGS_NONE());
    mrb_define_method(mrb, cHandle, "close", mrb_fiddle_handle_close, MRB_ARGS_NONE());
    mrb_define_method(mrb, cHandle, "sym",  mrb_fiddle_handle_sym, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cHandle, "[]",  mrb_fiddle_handle_sym,  MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cHandle, "disable_close", mrb_fiddle_handle_disable_close, MRB_ARGS_NONE());
    mrb_define_method(mrb, cHandle, "enable_close", mrb_fiddle_handle_enable_close, MRB_ARGS_NONE());
    mrb_define_method(mrb, cHandle, "close_enabled?", mrb_fiddle_handle_close_enabled_p, MRB_ARGS_NONE());
}

/* vim: set noet sws=4 sw=4: */
