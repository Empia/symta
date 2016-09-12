#ifndef RUNTIME_INTERNAL_H
#define RUNTIME_INTERNAL_H

#include "symta.h"

#define MAX_METHODS (4*1024)
#define MAX_LIBS 1024

// predefine method slots
#define M_SIZE 0
#define M_NAME 1
#define M_SINK 2

#define LIST_SIZE(o) ((uintptr_t)O_CODE(o))

#define IS_FIXTEXT(o) (O_TAGL(o) == T_FIXTEXT)
#define IS_BIGTEXT(o) (O_TAG(o) == TAG(T_TEXT))
#define IS_TEXT(o) (IS_FIXTEXT(o) || IS_BIGTEXT(o))

#define BIGTEXT_SIZE(o) REF4(o,0)
#define BIGTEXT_DATA(o) ((char*)&REF1(o,4))

#define C_ANY(o,arg_index,meta)

#define C_FN(o,arg_index,meta) \
  if (O_TAG(o) != TAG(T_CLOSURE)) \
    api->bad_type(REGS_ARGS(P), "fn", arg_index, meta)

#define C_INT(o,arg_index,meta) \
  if (O_TAGL(o) != T_INT) \
    api->bad_type(REGS_ARGS(P), "int", arg_index, meta)

#define C_FLOAT(o,arg_index,meta) \
  if (O_TAGL(o) != T_FLOAT) \
    api->bad_type(REGS_ARGS(P), "float", arg_index, meta)

#define C_TEXT(o,arg_index,meta) \
  if (!IS_TEXT(o)) \
    api->bad_type(REGS_ARGS(P), "text", arg_index, meta)

#define C_BYTES(o,arg_index,meta) \
  if (O_TAG(o) != TAG(T_BYTES)) \
    api->bad_type(REGS_ARGS(P), "bytes", arg_index, meta)

#define BUILTIN_CLOSURE(dst,code) { ALLOC_CLOSURE(dst, code, 0); }


#define BUILTIN_CHECK_NARGS(expected,tag,name) \
  if (NARGS(E) != FIXNUM(expected)) { \
    void *meta, *ttag, *t; \
    LIST_ALLOC(meta, 1); \
    TEXT(t, name); \
    REF(meta,0) = t; \
    if (tag) { \
      TEXT(ttag, tag); \
    } else { \
      ttag = No; \
    } \
    return api->handle_args(REGS_ARGS(P), E, FIXNUM(expected), FIXNUM(0), ttag, meta); \
  }
#define BUILTIN_CHECK_VARARGS(expected,tag,name) \
  if (NARGS(E) < FIXNUM(expected)) { \
    void *meta, *ttag, *t; \
    LIST_ALLOC(meta, 1); \
    TEXT(t, name); \
    REF(meta,0) = t; \
    if (tag) { \
      TEXT(ttag, tag); \
    } else { \
      ttag = No; \
    } \
    return api->handle_args(REGS_ARGS(P), E, -FIXNUM(expected), FIXNUM(0), ttag, meta); \
  }

#define BUILTIN_SETUP(name,nargs) \
  static void *b_##name(REGS); \
  static fn_meta_t meta_b_##name; \
  static void setup_b_##name(api_t *api) { \
    void *p = b_##name; \
    FNMETA(p,meta_b_##name,0,nargs); \
  }

#define BUILTIN0(sname, name) \
  BUILTIN_SETUP(name,0) \
  static void *b_##name(REGS) { \
  PROLOGUE; \
  void *A, *R; \
  BUILTIN_CHECK_NARGS(0,0,sname);
#define BUILTIN1(sname,name,a_check,a) \
  BUILTIN_SETUP(name,1) \
  static void *b_##name(REGS) { \
  PROLOGUE; \
  void *A, *R, *a; \
  BUILTIN_CHECK_NARGS(1,0,sname); \
  a = getArg(0); \
  a_check(a, 0, sname);
#define BUILTIN2(sname,name,a_check,a,b_check,b) \
  BUILTIN_SETUP(name,2) \
  static void *b_##name(REGS) { \
  PROLOGUE; \
  void *A, *R, *a, *b; \
  BUILTIN_CHECK_NARGS(2,0,sname); \
  a = getArg(0); \
  a_check(a, 0, sname); \
  b = getArg(1); \
  b_check(b, 1, sname);
#define BUILTIN3(sname,name,a_check,a,b_check,b,c_check,c) \
  BUILTIN_SETUP(name,3) \
  static void *b_##name(REGS) { \
  PROLOGUE; \
  void *A, *R, *a, *b,*c; \
  BUILTIN_CHECK_NARGS(3,0,sname); \
  a = getArg(0); \
  a_check(a, 0, sname); \
  b = getArg(1); \
  b_check(b, 1, sname); \
  c = getArg(2); \
  c_check(c, 2, sname);
#define BUILTIN_VARARGS(sname,name) \
  BUILTIN_SETUP(name,-1) \
  static void *b_##name(REGS) { \
  PROLOGUE; \
  void *A, *R; \
  BUILTIN_CHECK_VARARGS(0,0,sname);
#define RETURNS(r) R = (void*)(r); RETURN(R); }
#define RETURNS_NO_GC(r) RETURN_NO_GC(r); }


#endif //SYMTA_H
