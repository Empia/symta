#ifndef SYMTA_H
#define SYMTA_H

#include <stdint.h>
#include <setjmp.h>

#define SYMTA_DEBUG 1

#define TAGL_BITS ((uintptr_t)2)
#define PTR_BITS ((uintptr_t)40)
#define TAGH_BITS ((uintptr_t)10)
#define ALIGN_BITS ((uintptr_t)3)

//#define MAX_TYPES (1<<TAGH_BITS)
#define MAX_TYPES 512

#define TAGH_SHIFT (64-TAGH_BITS)

#define TAGL_MASK (((uintptr_t)1<<TAGL_BITS)-1)
#define PTR_MASK ((((uintptr_t)1<<PTR_BITS)-1)<<ALIGN_BITS)
#define TAGH_MASK ((((uintptr_t)1<<TAGH_BITS)-1)<<TAGH_SHIFT)
#define ALIGN_MASK (((uintptr_t)1<<ALIGN_BITS)-1)

#define LIFT_FLAG ((uintptr_t)4)

#define O_TAGL(o) ((uintptr_t)(o)&TAGL_MASK)
#define O_TAGH(o) ((uintptr_t)(o)>>TAGH_SHIFT)
#define O_TAG(o) ((uintptr_t)(o)&(TAGH_MASK|TAGL_MASK))
#define O_TYPE(o) (O_TAGL(o) == T_DATA ? O_TAGH(o) : O_TAGL(o))
#define O_PTR(o) ((uintptr_t)(o)&PTR_MASK)
#define REF1(base,off) (*(uint8_t*)(O_PTR(base)+(off)))
#define REF4(base,off) (*(uint32_t*)(O_PTR(base)+(off)*4))
#define REF(base,off) (*(void**)(O_PTR(base)+(off)*sizeof(void*)))
#define O_CODE(x) REF(x,-2)
#define O_LEVEL(x) ((uintptr_t)REF(x,-1))
#define O_FN(x) ((pfun)O_CODE(x))

#define NARGS(x) ((intptr_t)O_CODE(x))

#define TAG(tag) (((uintptr_t)(tag)<<TAGH_SHIFT) | T_DATA)

#define ADD_TAGL(src,tag) ((void*)((uintptr_t)(src) | tag))
#define ADD_TAG(src,tag) ((void*)((uintptr_t)(src) | TAG(tag)))

#define IMMEDIATE(x) (O_TAGL(x) != T_DATA)


#define T_INT     0
#define T_FLOAT   1
#define T_FIXTEXT 2
#define T_DATA    3
#define T_CLOSURE 4
#define T_LIST    5
#define T_VIEW    6
#define T_CONS    7
#define T_OBJECT  8
#define T_TEXT    9
#define T_VOID    10
#define T_GENERIC_LIST 11
#define T_GENERIC_TEXT 12
#define T_HARD_LIST    13
#define T_NAME         14
#define T_NAME_TEXT    15
#define T_BYTES        16

// sign preserving shifts
#define ASHL(x,count) ((x)*(1<<(count)))
#define ASHR(x,count) ((x)/(1<<(count)))
#define FIXNUM(x) ASHL((intptr_t)(x),ALIGN_BITS)
#define UNFIXNUM(x) ASHR((intptr_t)(x),ALIGN_BITS)

#define FXNNEG(dst,o) dst = (void*)(-(intptr_t)(o))
#define FXNADD(dst,a,b) dst = (void*)((intptr_t)(a) + (intptr_t)(b))
#define FXNSUB(dst,a,b) dst = (void*)((intptr_t)(a) - (intptr_t)(b))
#define FXNMUL(dst,a,b) dst = (void*)(UNFIXNUM(a) * (intptr_t)(b))
#define FXNDIV(dst,a,b) dst = (void*)(FIXNUM((intptr_t)(a) / (intptr_t)(b)))
#define FXNREM(dst,a,b) dst = (void*)((intptr_t)(a) % (intptr_t)(b))
#define FXNEQ(dst,a,b) dst = (void*)FIXNUM((intptr_t)(a) == (intptr_t)(b))
#define FXNNE(dst,a,b) dst = (void*)FIXNUM((intptr_t)(a) != (intptr_t)(b))
#define FXNLT(dst,a,b) dst = (void*)FIXNUM((intptr_t)(a) < (intptr_t)(b))
#define FXNGT(dst,a,b) dst = (void*)FIXNUM((intptr_t)(a) > (intptr_t)(b))
#define FXNLTE(dst,a,b) dst = (void*)FIXNUM((intptr_t)(a) <= (intptr_t)(b))
#define FXNGTE(dst,a,b) dst = (void*)FIXNUM((intptr_t)(a) >= (intptr_t)(b))
#define FXNTAG(dst,x) dst = (void*)FIXNUM(O_TAGL(x))
#define UNFXN(dst,x) dst = (void*)UNFIXNUM(x)

#define LOAD_FLOAT(dst,x) { \
    double d_ = (double)(x); \
    uint64_t t_ = (*(uint64_t*)&d_); \
    dst = (void*)((t_&~TAGL_MASK) | T_FLOAT); \
  }

#define UNFLOAT(dst,x) { \
    uint64_t t_ = (uint64_t)(x)&~TAGL_MASK; \
    dst = *(double*)&t_; \
  }

#define HEAP_SIZE (32*1024*1024)
#define OBJ_HEAD_SIZE 2

// should be less than C's stack, which is 1024*1024 bytes
#define MAX_LEVEL (1024*1024/9)

// P holds points to closure of current function
// E holds pointer to arglist of current function
#define REGS void *P, struct api_t *api
#define REGS_ARGS(P) P, api

typedef struct frame_t {
  void *base;  //pointer to heap
  void *mark;  //current function name
  void *lifts; //what should be lifted to parent frame
} frame_t;

#define PAGE_SIZE 4096

typedef struct api_t {
  frame_t frame[MAX_LEVEL];
  uint8_t frame_guard[PAGE_SIZE*2];
  void *top[2]; // heap top

  intptr_t level; // stack frame depth

  void *method; // current method, we execute

  void *jmp_return;

  // constants
  void *void_;
  void *empty_;
  void *m_ampersand;
  void *m_underscore;

  // runtime's C API
  void (*bad_type)(REGS, char *expected, int arg_index, char *name);
  void* (*handle_args)(REGS, void *E, intptr_t expected, intptr_t size, void *tag, void *meta);
  void (*set_meta)(void *addr, void *meta);
  void *(*get_meta)(void *addr);
  char* (*print_object_f)(struct api_t *api, void *object);
  void (*gc_lifts)();
  void *(*alloc_text)(char *s);
  void (*fatal)(struct api_t *api, void *msg);
  void (*fatal_chars)(struct api_t *api, char *msg);
  void **(*resolve_method)(struct api_t *api, char *name);
  int (*resolve_type)(struct api_t *api, char *name);
  void (*add_subtype)(struct api_t *api, intptr_t super, intptr_t sub);
  void (*set_type_size_and_name)(struct api_t *api, intptr_t tag, intptr_t size, void *name);
  void (*set_method)(struct api_t *api, void *method, void *type, void *handler);
  void *(*find_export)(struct api_t *api, void *name, void *exports);
  void *(*load_lib)(struct api_t *api, char *name);
  char *(*text_chars)(struct api_t *api, void *text);

  void *collectors[MAX_TYPES];

  void *heap[2][HEAP_SIZE];
} api_t;

typedef void *(*pfun)(REGS);

#define No api->void_
#define Empty api->empty_
#define Frame api->frame[Level]
#define Lifts Frame.lifts
#define Top api->top[Level&1]
#define Base Frame.base
#define Level api->level



//following could proable be useful, when allocating
// large memory size, that could jump over guard
#define HEAP_GUARD()
#define LIFT_GUARD()


#define ALLOC_BASIC(dst,code,count) \
  HEAP_GUARD(); \
  dst = (void**)Top - (uintptr_t)(count); \
  Top = (void**)dst - OBJ_HEAD_SIZE; \
  *((void**)Top+0) = (void*)(code); \
  *((void**)Top+1) = (void*)Level;

#define ALLOC_NO_CODE(dst,count) \
  HEAP_GUARD(); \
  dst = (void**)Top - (uintptr_t)(count); \
  Top = (void**)dst - 1; \
  *((void**)Top+0) = (void*)Level;

#define CLOSURE(dst,code,count) \
  ALLOC_BASIC(dst,code,count); \
  dst = ADD_TAG(dst, T_CLOSURE);

//local closure
#define LOSURE(dst,size) CLOSURE(dst,FIXNUM(size),size)

#define ALLOC_DATA(dst,tag,count) \
  ALLOC_NO_CODE(dst,count); \
  dst = ADD_TAG(dst,tag);

#define ARL(dst,size) ALLOC_BASIC(dst,FIXNUM(size),size)

#define LIST_ALLOC(dst,size) \
  ARL(dst,size); \
  dst = ADD_TAG(dst, T_LIST);

#define LOAD_LIB(dst,name) dst = api->load_lib(api,(char*)(name));
#define FIND_EXPORT(dst,symbol,lib) dst = api->find_export(api,symbol,lib);
#define RESOLVE_TYPE(dst,name) \
  dst = (void*)(intptr_t)api->resolve_type(api, (char*)(name));
#define RESOLVE_METHOD(dst,name) dst = api->resolve_method(api, name);
#define SET_TYPE_SIZE_AND_NAME(tag,size,name) api->set_type_size_and_name(api,tag,size,name);
#define DMET(method,type,handler) api->set_method(api,method,type,handler);
#define SUBTYPE(super,sub) api->add_subtype(api,(intptr_t)(super),(intptr_t)(sub));

#define IS_LIST(o) (O_TAG(o) == TAG(T_LIST))

#define print_object(object) api->print_object_f(api, object)
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define LOCAL(name) name:;
#define BRANCH(cnd,name) if (cnd) goto name;
#define ZBRANCH(cnd,name) if (!(cnd)) goto name;
#define JMP(name) goto name;
#ifdef WINDOWS
#define BEGIN_CODE \
  int __stdcall DllMainCRTStartup(void *a, uint32_t  b, void *c) { return 1; } \
  static void __dummy___ () {
#else
#define BEGIN_CODE static void __dummy___ () {
#endif
#define END_CODE }
#define LDFXN(dst,x) dst = (void*)FIXNUM(x)
#define TEXT(dst,x) dst = api->alloc_text((char*)(x))
#define THIS_METHOD(dst) dst = api->method;
#define METHOD_NAME(dst,method) dst = ((void**)(method))[T_NAME_TEXT];
#define TYPE_ID(dst,o) dst = (void*)FIXNUM(O_TYPE(o));
#define getArg(i) (*((void**)E+(i)))
#define PROLOGUE void *E = (void**)Top+OBJ_HEAD_SIZE;
#define ENTRY(name) } void *name(REGS) {PROLOGUE; void *dummy;
#define DECL_LABEL(name) static void *name(REGS);
#define LABEL(name) } static void *name(REGS) {PROLOGUE; void *dummy;
#define VAR(name) void *name;


typedef struct fn_meta_t {
  intptr_t size;    // closure size - the size of environment,
                    // this function closes over.
  void *nargs; // number of arguments
  void *name;  // function name text
  void *data;  // user-provided metadata
} fn_meta_t;

#define FNMETA(addr,meta,asize,anargs) \
  meta.size = asize; \
  LDFXN(meta.nargs, anargs); \
  api->set_meta(addr,&meta);

#define MARK(name) Frame.mark = (void*)(name);
#define BPUSH() \
  ++Level; \
  MARK(0); \
  /*fprintf(stderr, "Entering %ld\n", Level);*/ \
  Base = Top;
#define BPOP() \
  /*fprintf(stderr, "Leaving %ld\n", Level);*/ \
  Top = Base; \
  --Level;
#define CALL(k,f) k = O_FN(f)(REGS_ARGS(f));
#define MCALL_NO_SAVE(k,o,m) \
   { \
      void *f_; \
      f_ = ((void**)(m))[O_TYPE(o)]; \
      CALL(k,f_); \
   }
//method call
#define MCALL(k,o,m) \
   api->method = m; \
   MCALL_NO_SAVE(k,o,m);
#define CALL_TAGGED(k,o) \
  { \
    if (O_TAG(o) == TAG(T_CLOSURE)) { \
      CALL(k,o); \
    } else { \
      void *as = ADD_TAG((void**)Top+OBJ_HEAD_SIZE, T_LIST); \
      void *e; \
      ARL(e,2); \
      STARG(e,0,o); \
      STARG(e,1,as); \
      MCALL(k,o,api->m_ampersand); \
    } \
  }
typedef void *(*collector_t)( void *o);
#define GC_PARAM(dst,o,gclevel,pre,post) \
  { \
    void *o_ = (void*)(o); \
    if (IMMEDIATE(o_)) { \
      dst = o_; \
    } else { \
      uintptr_t level_ = O_LEVEL(o_); \
      if (level_ != gclevel) { \
        if (level_ > HEAP_SIZE) { \
          dst = (void*)level_; \
        } else { \
          dst = o_; \
        } \
      } else { \
        pre; \
        dst = ((collector_t)api->collectors[O_TYPE(o_)])(o_); \
        post; \
      } \
    } \
  }
#define GC(dst,value) \
  /*fprintf(stderr, "GC %p:%p -> %p\n", Top, Base, api->top[(Level-1)&1]);*/ \
  if (Lifts) api->gc_lifts(); \
  GC_PARAM(dst, value, Level, --Level, ++Level);
#define RETURN_NO_POP(value) \
   GC(value,value); \
   return (void*)(value);
#define RETURN(value) \
   GC(value,value); \
   BPOP(); \
   return (void*)(value);
#define RETURN_NO_GC(value) return (void*)(value);
#define LIFTS_CONS(dst,head,tail) \
  LIFT_GUARD(); \
  Top=(void**)Top-2; \
  *((void**)Top+0) = (head); \
  *((void**)Top+1) = (tail); \
  dst = Top;
#define LIFTS_HEAD(xs) (*((void**)(xs)+0))
#define LIFTS_TAIL(xs) (*((void**)(xs)+1))
#define LIFT(base,pos,value) \
  { \
    void **p_ = (void**)(base)+(pos); \
    if (IMMEDIATE(value)) { \
      *p_ = (value); \
    } else if (O_LEVEL(value) <= O_LEVEL(base)) { \
      *p_ = (void*)((uintptr_t)(value) & ~LIFT_FLAG); \
    } else { \
      *p_ = (void*)((uintptr_t)(value) | LIFT_FLAG); \
      LIFTS_CONS(Lifts, p_, Lifts); \
    } \
  }
#define LDARG(dst,src,src_off) dst = *((void**)(src)+(src_off))
#define STARG(dst,dst_off,src) *((void**)(dst)+(dst_off)) = (void*)(src)
#define LOAD(dst,src,src_off) dst = REF(src,src_off)
#define STOR(dst,dst_off,src) REF(dst,dst_off) = (void*)(src)
#define COPY(dst,dst_off,src,src_off) REF(dst,dst_off) = REF(src,src_off)
#define MOVE(dst,src) dst = (void*)(src)
#define TAGGED(dst,src,tag) dst = ADD_TAG(src,tag)
#define DATA_SET(dst,dst_off,src) LIFT(&REF(dst,0),dst_off,src)
#define DGET(dst,src,off) dst = REF(src, off)
#define DSET(dst,off,src) DATA_SET(dst, off, src)
#define DINIT(dst,off,src) REF(dst, off) = src
//untagged store
#define UTSTOR(dst,off,src) *(void**)((uint8_t*)(dst)+(uint64_t)(off)) = src

#define FATAL(msg) api->fatal(api, msg);

#define SET_UNWIND_HANDLER(r,h) Frame.mark = h;
#define REMOVE_UNWIND_HANDLER(r) Frame.mark = 0;

typedef struct {
  jmp_buf anchor;
  intptr_t level;
  api_t *api;
} jmp_state;

#define SETJMP(dst) { \
    jmp_state *js_; \
    ALLOC_BASIC(api->jmp_return, 0, ((sizeof(jmp_state)+ALIGN_MASK)>>ALIGN_BITS)); \
    js_ = (jmp_state*)api->jmp_return; \
    js_->level = api->level; \
    js_->api = api; \
    setjmp(js_->anchor); \
    api = js_->api; \
    dst = api->jmp_return; \
  }

#define LONGJMP(state,value) { \
    jmp_state *js_; \
    js_ = (jmp_state*)state; \
    while (js_->level != api->level) { \
      void *h_ = Frame.mark; \
      if (O_TAG(h_) == TAG(T_CLOSURE)) { \
          void *k_; \
          BPUSH(); \
          ARL(E,0); \
          CALL(k_,h_) \
      } \
      GC(value,value); \
      BPOP(); \
    } \
    api->jmp_return = value; \
    longjmp(js_->anchor, 0); \
  }

#define CHECK_NARGS(expected,size,meta) \
  if (NARGS(E) != FIXNUM(expected)) { \
    return api->handle_args(REGS_ARGS(P), E, FIXNUM(expected), FIXNUM(size), No, meta); \
  }
#define CHECK_VARARGS(size,meta) \
  if (NARGS(E) < FIXNUM(0)) { \
    return api->handle_args(REGS_ARGS(P), E, FIXNUM(-1), FIXNUM(size), No, meta); \
  }

// kludge for FFI identifiers
#define text_ char*
#define voidp_ void*
#define u4 uint32_t

#define FFI_VAR(type,name) type name;

#define FFI_TO_INT(dst,src) \
  if (O_TAGL(src) != T_INT) \
    api->bad_type(REGS_ARGS(P), "int", 0, 0); \
  dst = (int)UNFIXNUM(src);
#define FFI_FROM_INT(dst,src) dst = (void*)FIXNUM((intptr_t)src);

#define FFI_TO_U4(dst,src) \
  if (O_TAGL(src) != T_INT) \
    api->bad_type(REGS_ARGS(P), "int", 0, 0); \
  dst = (uint32_t)UNFIXNUM(src);
#define FFI_FROM_U4(dst,src) dst = (void*)FIXNUM((intptr_t)src);

#define FFI_TO_DOUBLE(dst,src) UNFLOAT(dst,src);
#define FFI_FROM_DOUBLE(dst,src) LOAD_FLOAT(dst,src);

#define FFI_TO_FLOAT(dst,src) { \
    double _x; \
    UNFLOAT(_x,src); \
    _dst = (float)_x; \
  }
#define FFI_FROM_FLOAT(dst,src) LOAD_FLOAT(dst,(double)src);

#define FFI_TO_TEXT_(dst,src) dst = api->text_chars(api,src);
#define FFI_FROM_TEXT_(dst,src) dst = api->alloc_text(src);

#define FFI_TO_VOIDP_(dst,src) dst = (void*)(src);
#define FFI_FROM_VOIDP_(dst,src) dst = (void*)(src);

#define FFI_GET(dst,type,ptr,off) dst = (void*)FIXNUM(((type*)(ptr))[UNFIXNUM(off)]);
#define FFI_SET(type,ptr,off,val) ((type*)(ptr))[UNFIXNUM(off)] = (type)UNFIXNUM(val);

void *entry(REGS);

#endif //SYMTA_H
