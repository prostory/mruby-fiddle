#include "fiddle.h"

struct RClass *cFiddle;
struct RClass *cFiddleError;

extern void mrb_fiddle_pointer_init(mrb_state *mrb);
extern void mrb_fiddle_function_init(mrb_state *mrb);
extern void mrb_fiddle_handle_init(mrb_state *mrb);
extern void mrb_fiddle_closure_init(mrb_state *mrb);
extern void mrb_fiddle_memory_trace_init(mrb_state *mrb);

/*
 * call-seq: Fiddle.malloc(size)
 *
 * Allocate +size+ bytes of memory and return the integer memory address
 * for the allocated memory.
 */
static mrb_value
mrb_fiddle_malloc(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    mrb_int size;

    mrb_get_args(mrb, "i", &size);

    ptr = (void *)mrb_malloc(mrb, size);
    return mrb_cptr_value(mrb, ptr);
}

/*
 * call-seq: Fiddle.calloc(size)
 *
 * Allocate +size+ bytes of memory and return the integer memory address
 * for the allocated memory.
 */
static mrb_value
mrb_fiddle_calloc(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    mrb_int nelem, size;

    mrb_get_args(mrb, "ii", &nelem, &size);

    ptr = (void *)mrb_calloc(mrb, nelem, size);
    return mrb_cptr_value(mrb, ptr);
}

/*
 * call-seq: Fiddle.realloc(addr, size)
 *
 * Change the size of the memory allocated at the memory location +addr+ to
 * +size+ bytes.  Returns the memory address of the reallocated memory, which
 * may be different than the address passed in.
 */
static mrb_value
mrb_fiddle_realloc(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    mrb_value addr;
    mrb_int size;

    mrb_get_args(mrb, "oi", &addr, &size);

    ptr = mrb_cptr(addr);

    ptr = (void*)mrb_realloc(mrb, ptr, size);
    return mrb_cptr_value(mrb, ptr);
}

/*
 * call-seq: Fiddle.free(addr)
 *
 * Free the memory at address +addr+
 */
static mrb_value
mrb_fiddle_free(mrb_state *mrb, mrb_value self)
{
    void *ptr;
    mrb_value addr;

    mrb_get_args(mrb, "o", &addr);
    ptr = mrb_cptr(addr);

    mrb_free(mrb, ptr);
    return mrb_nil_value();
}

/*
 * call-seq: Fiddle.dlunwrap(addr)
 *
 * Returns the hexadecimal representation of a memory pointer address +addr+
 *
 * Example:
 *
 *   lib = Fiddle.dlopen('/lib64/libc-2.15.so')
 *   => #<Fiddle::Handle:0x00000001342460>
 *
 *   lib['strcpy'].to_s(16)
 *   => "7f59de6dd240"
 *
 *   Fiddle.dlunwrap(Fiddle.dlwrap(lib['strcpy'].to_s(16)))
 *   => "7f59de6dd240"
 */
static mrb_value
mrb_fiddle_ptr2value(mrb_state *mrb, mrb_value self)
{
    mrb_int addr;

    mrb_get_args(mrb, "i", &addr);

    return mrb_cptr_value(mrb, (void *)addr);
}

/*
 * call-seq: Fiddle.dlwrap(val)
 *
 * Returns a memory pointer of a function's hexadecimal address location +val+
 *
 * Example:
 *
 *   lib = Fiddle.dlopen('/lib64/libc-2.15.so')
 *   => #<Fiddle::Handle:0x00000001342460>
 *
 *   Fiddle.dlwrap(lib['strcpy'].to_s(16))
 *   => 25522520
 */
static mrb_value
mrb_fiddle_value2ptr(mrb_state *mrb, mrb_value self)
{
    mrb_int ptr;
    mrb_value addr;

    mrb_get_args(mrb, "o", &addr);
    ptr = (int)mrb_cptr(addr);

    return mrb_fixnum_value(ptr);
}

static void
mrb_fiddle_init(mrb_state *mrb)
{
    /*
     * Document-module: Fiddle
     *
     * A libffi wrapper for Ruby.
     *
     * == Description
     *
     * Fiddle is an extension to translate a foreign function interface (FFI)
     * with ruby.
     *
     * It wraps {libffi}[http://sourceware.org/libffi/], a popular C library
     * which provides a portable interface that allows code written in one
     * language to call code written in another language.
     *
     * == Example
     *
     * Here we will use Fiddle::Function to wrap {floor(3) from
     * libm}[http://linux.die.net/man/3/floor]
     *
     *	    require 'cFiddle'
     *
     *	    libm = Fiddle.dlopen('/lib/libm.so.6')
     *
     *	    floor = Fiddle::Function.new(
     *	      libm['floor'],
     *	      [Fiddle::TYPE_DOUBLE],
     *	      Fiddle::TYPE_DOUBLE
     *	    )
     *
     *	    puts floor.call(3.14159) #=> 3.0
     *
     *
     */
    cFiddle = mrb_define_module(mrb, "Fiddle");

    /*
     * Document-class: Fiddle::DLError
     *
     * standard dynamic load exception
     */
    cFiddleError = mrb_define_class_under(mrb, cFiddle, "DLError", mrb->eStandardError_class);

    /* Document-const: TYPE_VOID
     *
     * C type - void
     */
    mrb_define_const(mrb, cFiddle, "TYPE_VOID",      mrb_fixnum_value(TYPE_VOID));

    /* Document-const: TYPE_VOIDP
     *
     * C type - void*
     */
    mrb_define_const(mrb, cFiddle, "TYPE_VOIDP",     mrb_fixnum_value(TYPE_VOIDP));

    /* Document-const: TYPE_CHAR
     *
     * C type - char
     */
    mrb_define_const(mrb, cFiddle, "TYPE_CHAR",      mrb_fixnum_value(TYPE_CHAR));

    /* Document-const: TYPE_SHORT
     *
     * C type - short
     */
    mrb_define_const(mrb, cFiddle, "TYPE_SHORT",     mrb_fixnum_value(TYPE_SHORT));

    /* Document-const: TYPE_INT
     *
     * C type - int
     */
    mrb_define_const(mrb, cFiddle, "TYPE_INT",       mrb_fixnum_value(TYPE_INT));

    /* Document-const: TYPE_LONG
     *
     * C type - long
     */
    mrb_define_const(mrb, cFiddle, "TYPE_LONG",      mrb_fixnum_value(TYPE_LONG));

#if HAVE_LONG_LONG
    /* Document-const: TYPE_LONG_LONG
     *
     * C type - long long
     */
    mrb_define_const(mrb, cFiddle, "TYPE_LONG_LONG", mrb_fixnum_value(TYPE_LONG_LONG));
#endif

    /* Document-const: TYPE_FLOAT
     *
     * C type - float
     */
    mrb_define_const(mrb, cFiddle, "TYPE_FLOAT",     mrb_fixnum_value(TYPE_FLOAT));

    /* Document-const: TYPE_DOUBLE
     *
     * C type - double
     */
    mrb_define_const(mrb, cFiddle, "TYPE_DOUBLE",    mrb_fixnum_value(TYPE_DOUBLE));

    /* Document-const: ALIGN_VOIDP
     *
     * The alignment size of a void*
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_VOIDP", mrb_fixnum_value(ALIGN_VOIDP));

    /* Document-const: ALIGN_CHAR
     *
     * The alignment size of a char
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_CHAR",  mrb_fixnum_value(ALIGN_CHAR));

    /* Document-const: ALIGN_SHORT
     *
     * The alignment size of a short
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_SHORT", mrb_fixnum_value(ALIGN_SHORT));

    /* Document-const: ALIGN_INT
     *
     * The alignment size of an int
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_INT",   mrb_fixnum_value(ALIGN_INT));

    /* Document-const: ALIGN_LONG
     *
     * The alignment size of a long
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_LONG",  mrb_fixnum_value(ALIGN_LONG));

#if HAVE_LONG_LONG
    /* Document-const: ALIGN_LONG_LONG
     *
     * The alignment size of a long long
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_LONG_LONG",  mrb_fixnum_value(ALIGN_LONG_LONG));
#endif

    /* Document-const: ALIGN_FLOAT
     *
     * The alignment size of a float
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_FLOAT", mrb_fixnum_value(ALIGN_FLOAT));

    /* Document-const: ALIGN_DOUBLE
     *
     * The alignment size of a double
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_DOUBLE",mrb_fixnum_value(ALIGN_DOUBLE));

    /* Document-const: ALIGN_SIZE_T
     *
     * The alignment size of a size_t
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_SIZE_T", mrb_fixnum_value(ALIGN_OF(size_t)));

    /* Document-const: ALIGN_SSIZE_T
     *
     * The alignment size of a ssize_t
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_SSIZE_T", mrb_fixnum_value(ALIGN_OF(size_t))); /* same as size_t */

    /* Document-const: ALIGN_PTRDIFF_T
     *
     * The alignment size of a ptrdiff_t
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_PTRDIFF_T", mrb_fixnum_value(ALIGN_OF(ptrdiff_t)));

    /* Document-const: ALIGN_INTPTR_T
     *
     * The alignment size of a intptr_t
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_INTPTR_T", mrb_fixnum_value(ALIGN_OF(intptr_t)));

    /* Document-const: ALIGN_UINTPTR_T
     *
     * The alignment size of a uintptr_t
     */
    mrb_define_const(mrb, cFiddle, "ALIGN_UINTPTR_T", mrb_fixnum_value(ALIGN_OF(uintptr_t)));

    /* Document-const: WINDOWS
     *
     * Returns a boolean regarding whether the host is WIN32
     */
#if defined(_WIN32)
    mrb_define_const(mrb, cFiddle, "WINDOWS", mrb_true_value());
#else
    mrb_define_const(mrb, cFiddle, "WINDOWS", mrb_false_value());
#endif

    /* Document-const: SIZEOF_VOIDP
     *
     * size of a void*
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_VOIDP", mrb_fixnum_value(sizeof(void*)));

    /* Document-const: SIZEOF_CHAR
     *
     * size of a char
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_CHAR",  mrb_fixnum_value(sizeof(char)));

    /* Document-const: SIZEOF_SHORT
     *
     * size of a short
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_SHORT", mrb_fixnum_value(sizeof(short)));

    /* Document-const: SIZEOF_INT
     *
     * size of an int
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_INT",   mrb_fixnum_value(sizeof(int)));

    /* Document-const: SIZEOF_LONG
     *
     * size of a long
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_LONG",  mrb_fixnum_value(sizeof(long)));

#if HAVE_LONG_LONG
    /* Document-const: SIZEOF_LONG_LONG
     *
     * size of a long long
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_LONG_LONG",  mrb_fixnum_value(sizeof(LONG_LONG)));
#endif

    /* Document-const: SIZEOF_FLOAT
     *
     * size of a float
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_FLOAT", mrb_fixnum_value(sizeof(float)));

    /* Document-const: SIZEOF_DOUBLE
     *
     * size of a double
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_DOUBLE",mrb_fixnum_value(sizeof(double)));

    /* Document-const: SIZEOF_SIZE_T
     *
     * size of a size_t
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_SIZE_T",  mrb_fixnum_value(sizeof(size_t)));

    /* Document-const: SIZEOF_SSIZE_T
     *
     * size of a ssize_t
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_SSIZE_T",  mrb_fixnum_value(sizeof(size_t))); /* same as size_t */

    /* Document-const: SIZEOF_PTRDIFF_T
     *
     * size of a ptrdiff_t
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_PTRDIFF_T",  mrb_fixnum_value(sizeof(ptrdiff_t)));

    /* Document-const: SIZEOF_INTPTR_T
     *
     * size of a intptr_t
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_INTPTR_T",  mrb_fixnum_value(sizeof(intptr_t)));

    /* Document-const: SIZEOF_UINTPTR_T
     *
     * size of a uintptr_t
     */
    mrb_define_const(mrb, cFiddle, "SIZEOF_UINTPTR_T",  mrb_fixnum_value(sizeof(uintptr_t)));

    /* Document-const: RUBY_FREE
     *
     * Address of the ruby_xfree() function
     */
    //mrb_define_const(mrb, cFiddle, "RUBY_FREE", mrb_cptr_value(mrb, ruby_xfree));

    /* Document-const: BUILD_RUBY_PLATFORM
     *
     * Platform built against (i.e. "x86_64-linux", etc.)
     *
     * See also RUBY_PLATFORM
     */
    //mrb_define_const(mrb, cFiddle, "BUILD_RUBY_PLATFORM", rb_str_new2(RUBY_PLATFORM));

    mrb_define_module_function(mrb, cFiddle, "dlwrap", mrb_fiddle_value2ptr, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, cFiddle, "dlunwrap", mrb_fiddle_ptr2value, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, cFiddle, "malloc", mrb_fiddle_malloc, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, cFiddle, "calloc", mrb_fiddle_calloc, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, cFiddle, "realloc", mrb_fiddle_realloc, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, cFiddle, "free", mrb_fiddle_free, MRB_ARGS_REQ(1));
}

extern void
memory_report(void);

void
mrb_mruby_fiddle_gem_init(mrb_state* mrb) {
    atexit(memory_report);
    mrb_fiddle_init(mrb);
    mrb_fiddle_pointer_init(mrb);
    mrb_fiddle_function_init(mrb);
    mrb_fiddle_handle_init(mrb);
    mrb_fiddle_closure_init(mrb);
    mrb_fiddle_memory_trace_init(mrb);
    mrb_gc_arena_restore(mrb, 0);
}

void
mrb_mruby_fiddle_gem_final(mrb_state* mrb) {
  /* finalizer */
}
/* vim: set noet sws=4 sw=4: */
