#ifndef FIDDLE_H
#define FIDDLE_H

#include <mruby.h>
#include <mruby/data.h>
#include <mruby/proc.h>
#include <mruby/class.h>
#include <mruby/value.h>
#include <mruby/array.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/variable.h>


#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if defined(HAVE_DLFCN_H)
# include <dlfcn.h>
# /* some stranger systems may not define all of these */
#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#else
# if defined(_WIN32)
#   include <windows.h>
#   define dlopen(name,flag) ((void*)LoadLibrary(name))
#   define dlerror() strerror(rb_w32_map_errno(GetLastError()))
#   define dlsym(handle,name) ((void*)GetProcAddress((handle),(name)))
#   define RTLD_LAZY -1
#   define RTLD_NOW  -1
#   define RTLD_GLOBAL -1
# endif
#endif

#ifdef USE_HEADER_HACKS
#include <ffi/ffi.h>
#else
#include <ffi.h>
#endif

#include "config.h"

#if HAVE_LONG_LONG
# if SIZEOF_LONG_LONG == 8
#   define ffi_type_slong_long ffi_type_sint64
#   define ffi_type_ulong_long ffi_type_uint64
# else
#  error "long long size not supported"
# endif
#endif

#define TYPE_VOID  0
#define TYPE_VOIDP 1
#define TYPE_CHAR  2
#define TYPE_SHORT 3
#define TYPE_INT   4
#define TYPE_LONG  5
#if HAVE_LONG_LONG
#define TYPE_LONG_LONG 6
#endif
#define TYPE_FLOAT 7
#define TYPE_DOUBLE 8

#define ALIGN_OF(type) offsetof(struct {char align_c; type align_x;}, align_x)

#define ALIGN_VOIDP  ALIGN_OF(void*)
#define ALIGN_SHORT  ALIGN_OF(short)
#define ALIGN_CHAR   ALIGN_OF(char)
#define ALIGN_INT    ALIGN_OF(int)
#define ALIGN_LONG   ALIGN_OF(long)
#if HAVE_LONG_LONG
#define ALIGN_LONG_LONG ALIGN_OF(LONG_LONG)
#endif
#define ALIGN_FLOAT  ALIGN_OF(float)
#define ALIGN_DOUBLE ALIGN_OF(double)

#include "memory.h"

#endif
/* vim: set noet sws=4 sw=4: */
