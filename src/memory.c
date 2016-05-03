#include <stdlib.h>
#include "fiddle.h"

#undef mrb_malloc
#undef mrb_free
#undef mrb_realloc
#undef mrb_calloc

typedef struct MemoryInfo MemoryInfo;
typedef struct MemoryTrace MemoryTrace;

struct MemoryInfo
{
  void *address;
  size_t size;
  char *file;
  int line;
  MemoryInfo *next;
};

struct MemoryTrace
{
  MemoryInfo *head;
  MemoryInfo *tail;
  size_t size;
};

static MemoryTrace trace = {0};

static MemoryInfo *
add_memory_info(mrb_state *mrb, void *address, size_t size, const char *file, int line)
{
  MemoryInfo *info;

  info = (MemoryInfo *)mrb_malloc(mrb, sizeof(MemoryInfo));
  info->address = address;
  info->size = size;
  info->file = file;
  info->line = line;
  info->next = NULL;

  if (trace.head == NULL) {
    trace.head = trace.tail = info;
  } else {
    trace.tail->next = info;
    trace.tail = trace.tail->next;
  }
  trace.size ++;

  return info;
}

static MemoryInfo *
del_memory_info(void *address) {
  MemoryInfo *pos = NULL;
  MemoryInfo *safe = NULL;

  for (pos = trace.head, safe = pos; pos && safe; safe = pos, pos = pos->next) {
    if (pos->address == address) {
      if (pos == trace.head) {
        trace.head = pos->next;
      } else if (pos == trace.tail) {
        trace.tail = safe;
      } else {
        safe->next = pos->next;
      }

      trace.size --;
      return pos;
    }
  }

  return NULL;
}

void *
xmalloc(mrb_state *mrb, size_t size, const char *file, int line)
{
  void *ptr = mrb_malloc(mrb, size);
  add_memory_info(mrb, ptr, size, file, line);
#ifdef MEMORY_INFO
  fprintf(stdout, "INFO: [%s:%d] malloc %lu bytes memory at %p\n", file, line, size, ptr);
#endif

  return ptr;
}

void
xfree(mrb_state *mrb, void *ptr, const char *file, int line)
{
  MemoryInfo *info = NULL;

  mrb_free(mrb, ptr);
  info = del_memory_info(ptr);
  if (info == NULL) {
    fprintf(stdout, "ERROR: [%s:%d] free NULL memory at %p\n", file, line, ptr);
  } else {
#ifdef MEMORY_INFO
    fprintf(stdout, "INFO: [%s:%d] free %lu bytes memory (allocated at [%s:%d]) at %p\n", file, line, 
      info->size, info->file, info->line, ptr);
#endif
    mrb_free(mrb, info);
  }
}

void *
xrealloc(mrb_state *mrb, void *ptr, size_t size, const char *file, int line)
{
  MemoryInfo *info = NULL;

  void *new_ptr = mrb_realloc(mrb, ptr, size);
  info = del_memory_info(ptr);
  if (info != NULL) {
    mrb_free(mrb, info);
  }
  add_memory_info(mrb, new_ptr, size, file, line);
#ifdef MEMORY_INFO
  fprintf(stdout, "INFO: [%s:%d] realloc %lu bytes memory at %p from %p\n", file, line, size, new_ptr, ptr);
#endif

  return new_ptr;
}

void *
xcalloc(mrb_state *mrb, size_t nmem, size_t size, const char *file, int line)
{
  void *ptr = mrb_calloc(mrb, nmem, size);
  add_memory_info(mrb, ptr, nmem * size, file, line);
#ifdef MEMORY_INFO
  fprintf(stdout, "INFO: [%s:%d] calloc %lu bytes memory at %p\n", file, line, nmem * size, ptr);
#endif
  return ptr;
}

void
memory_report(void)
{
  if (trace.size > 0) {
    size_t total_size = 0;
    MemoryInfo *next = trace.head;
    fprintf(stdout, "MemoryLeak: \n");
    while(next != NULL) {
      fprintf(stdout, "\tleak %lu bytes at %p allocated from [%s:%d]\n", next->size, next->address,
        next->file, next->line);
      total_size += next->size;
      next = next->next;
    }
    fprintf(stdout, "Leak memory total %lu bytes\n", total_size);
  }
}

static mrb_value
mrb_fiddle_memory_report(mrb_state *mrb, mrb_value self)
{
  memory_report();

  return mrb_nil_value();
}

extern struct RClass *cFiddle;

void
mrb_fiddle_memory_trace_init(mrb_state *mrb)
{
  mrb_define_module_function(mrb, cFiddle, "memory_report", mrb_fiddle_memory_report, MRB_ARGS_NONE());
}