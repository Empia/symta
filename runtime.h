#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// used for debugging
#define D fprintf(stderr, "%d:%s\n", __LINE__, __FILE__);

#define TAG_BITS ((uintptr_t)2)
#define TAG_MASK (((uintptr_t)1<<TAG_BITS)-1)
#define GET_TAG(x) ((uintptr_t)(x)&TAG_MASK)

#define T_CLOSURE  0
#define T_FIXNUM   1

//#define T_FLOAT
//#define T_NEXT_HERE
//#define T_NEXT_NONE

#define MAX_POOLS 1024*100
#define POOL_SIZE 64
#define POOL_BYTE_SIZE (POOL_SIZE*sizeof(void*))
#define POOL_MASK (uintptr_t)(POOL_BYTE_SIZE-1)
#define POOL_BASE (~POOL_MASK)
#define POOL_HANDLER(x) (((pfun*)((uintptr_t)(x)&POOL_BASE))[0])
#define TO_FIXNUM(x) (((uintptr_t)(x)*(1<<TAG_BITS)) + 1)

#define HEAP_SIZE (1024*1024*32)
#define MAX_ARRAY_SIZE (HEAP_SIZE/2)

typedef struct regs_t {
  // registers array
  void *E; // current environment
  void *P; // parent environment
  void *A; // args scratchpad
  void *C; // code pointer
  void *R; // return value
  void *T; // temporary, used by CLOSURE, CONS and other macros

  // constants
  void *Void;
  void *Empty;

  // utils
  void *fin;  // the closure, which would receive evaluation result
  void *run;  // the closure, which would receive the resulting program
  void *host; // called to resolve builtin functions (runtime API)

  // runtime's C API
  char *(*get_tag_name)(int tag);
  void (*handle_args)(struct regs_t *regs, intptr_t expected, void *tag, void *meta);
  char* (*print_object_f)(struct regs_t *regs, void *object);
  int (*new_pool)();
  void** (*alloc)(int count);
  void *(*alloc_text)(struct regs_t *regs, char *s);
  void (*fixnum)(struct regs_t *regs);
  void (*array)(struct regs_t *regs);

  // for multithreading, caching could be used to get on-demand pools for each thread, minimizing locking
  void **pools[MAX_POOLS];
} regs_t;


typedef void (*pfun)(regs_t *regs);

#define E regs->E
#define P regs->P
#define A regs->A
#define C regs->C
#define R regs->R
#define T regs->T
#define Void regs->Void
#define Empty regs->Empty
#define fin regs->fin
#define run regs->run
#define host regs->host

// number of arguments to the current function (size of E)
#define NARGS ((intptr_t)POOL_HANDLER(E))

#define print_object(object) regs->print_object_f(regs, object)
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define ALLOC(dst,code,pool,count) \
  MOVE(dst, regs->pools[pool]); \
  regs->pools[pool] += count; \
  if ((((uintptr_t)regs->pools[pool]+(count ? 0 : 1))&POOL_BASE) != ((uintptr_t)dst&POOL_BASE)) { \
    regs->pools[pool] = regs->alloc(count+1); \
    *regs->pools[pool]++ = (void*)(code); \
    if (((uintptr_t)regs->pools[pool]&POOL_MASK) || count>POOL_SIZE-1) { \
      MOVE(dst, regs->pools[pool]); \
      regs->pools[pool] += count; \
    } \
  }

#define ARRAY(dst,size) ALLOC(dst,TO_FIXNUM(size),MIN(POOL_SIZE,size),size)
#define FIXNUM(dst,x) dst = (void*)(((uintptr_t)(x)<<TAG_BITS) | T_FIXNUM)
#define UNFIXNUM(x) ((intptr_t)(x)/(1<<TAG_BITS))
#define TEXT(dst,x) dst = regs->alloc_text(regs,x)
#define CALL_BASE(f) POOL_HANDLER(f)(regs);
#define CALL(f) \
  MOVE(P, f); \
  CALL_BASE(f);
#define CALL_TAGGED(f) \
  if (GET_TAG(f) == T_CLOSURE) { \
    if ((intptr_t)POOL_HANDLER(f) < TO_FIXNUM(MAX_ARRAY_SIZE)) { \
      MOVE(P, f); \
      regs->array(regs); \
    } else { \
      CALL(f); \
    } \
  } else if (GET_TAG(f) == T_FIXNUM) { \
    MOVE(P, f); \
    regs->fixnum(regs); \
  } else { \
    printf("bad tag = %d\n", (int)GET_TAG(f)); \
    abort(); \
  }

#define STORE(dst,off,src) ((void**)(dst))[(int)(off)] = (void*)(src)
#define LOAD(dst,src,off) dst = ((void**)(src))[(int)(off)]
#define COPY(dst,p,src,q) ((void**)(dst))[(int)(p)] = ((void**)(src))[(int)(q)]
#define MOVE(dst,src) dst = (void*)(src)

#define CHECK_NARGS(expected,tag) \
  if (NARGS != TO_FIXNUM(expected)) { \
    regs->handle_args(regs, TO_FIXNUM(expected), tag, Empty); \
    return; \
  }
#define CHECK_VARARGS(tag) \
  if (NARGS < TO_FIXNUM(1)) { \
    regs->handle_args(regs, -1, tag, Empty); \
    return; \
  }

void entry(regs_t *regs);
