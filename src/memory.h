#ifndef FIDDLE_MEMORY_H
#define FIDDLE_MEMORY_H

#include <stdlib.h>
#include "fiddle.h"

extern void *
xmalloc(mrb_state *mrb, size_t size, const char *file, int line);

extern void
xfree(mrb_state *mrb, void *ptr, const char *file, int line);

extern void *
xrealloc(mrb_state *mrb, void *ptr, size_t size, const char *file, int line);

extern void *
xcalloc(mrb_state *mrb, size_t nmem, size_t size, const char *file, int line);


#ifdef MEMORY_TRACE
#define mrb_malloc(mrb, size) xmalloc(mrb, size, __FILE__, __LINE__)
#define mrb_free(mrb, ptr) xfree(mrb, ptr, __FILE__, __LINE__)
#define mrb_realloc(mrb, ptr, size) xrealloc(mrb, ptr, size, __FILE__, __LINE__)
#define mrb_calloc(mrb, nmem, size) xcalloc(mrb, nmem, size, __FILE__, __LINE__)
#endif

#endif