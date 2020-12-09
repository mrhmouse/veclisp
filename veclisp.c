/****************************************
 * veclisp.c                            *
 * a lisp with vectors, pairs, ints,    *
 * and symbols                          *
 *                                      *
 * author: jordan@yonder.computer       *
 * license: MIT                         *
 ****************************************/

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

#define TRACE(x)  fputs(x "\n", stderr)

struct veclisp_cell {
  enum
    { VECLISP_INT,
      VECLISP_SYM,
      VECLISP_VEC,
      VECLISP_PAIR,
    } type;
  union {
    int64_t integer;
    char *sym;
    struct veclisp_cell *vec;
    struct veclisp_cell *pair;
  } as;
};
struct veclisp_scope {
  struct veclisp_bindings {
    char *sym;
    struct veclisp_cell value;
    struct veclisp_bindings *next;
  } *bindings;
  struct veclisp_scope *next;
};
struct veclisp_interned_syms {
  char *sym;
  struct veclisp_interned_syms *next;
} *veclisp_interned_syms;
typedef int (*veclisp_native_func)(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);

char *VECLISP_T, *VECLISP_OUTPORT, *VECLISP_INPORT, *VECLISP_ERRPORT, *VECLISP_PROMPT, *VECLISP_DEFAULT_PROMPT, *VECLISP_QUOTE, *VECLISP_UNQUOTE, *VECLISP_RESPONSE, *VECLISP_DEFAULT_RESPONSE, *VECLISP_ERR_ILLEGAL_DOTTED_LIST, *VECLISP_ERR_EXPECTED_CLOSE_PAREN, *VECLISP_ERR_CANNOT_EXEC_VEC, *VECLISP_ERR_INVALID_NAME, *VECLISP_ERR_EXPECTED_PAIR, *VECLISP_ERR_ILLEGAL_LAMBDA_LIST, *VECLISP_ERR_EXPECTED_INT, *VECLISP_ERR_INVALID_SEQUENCE, *VECLISP_ERR_CANNOT_UPVAL_AT_TOPLEVEL;
char *veclisp_intern(char *sym);
void veclisp_print_prompt(struct veclisp_scope *scope);
void veclisp_write_result(struct veclisp_scope *scope, struct veclisp_cell value);
void veclisp_print_err(struct veclisp_scope *scope, struct veclisp_cell err);
int veclisp_init_root_scope(struct veclisp_scope *root_scope);
int veclisp_scope_lookup(struct veclisp_scope *scope, char *sym, struct veclisp_cell *result);
int veclisp_read(struct veclisp_scope *scope, struct veclisp_cell *result);
int veclisp_eval(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result);
int veclisp_lambda(struct veclisp_scope *parent_scope, struct veclisp_cell lambda, struct veclisp_cell args, struct veclisp_cell *result);
int veclisp_n_call(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result);
void veclisp_write(struct veclisp_scope *scope, struct veclisp_cell value);
void veclisp_set(struct veclisp_scope *scope, char *interned_sym, struct veclisp_cell value);
void veclisp_fwrite(FILE *out, struct veclisp_cell value);
int veclisp_n_quote(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_intp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_symp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_vecp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_pairp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_nilp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_pair(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_head(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_tail(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_cmp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_eq(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_gt(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_lt(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_gte(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_lte(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_set(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_syms(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_add(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_sub(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_mul(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_div(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_mod(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_exp(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_rsh(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_lsh(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_bitwiseand(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_bitwiseor(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_bitwisexor(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_bitwisenot(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_abs(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_sqrt(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_rand(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_and(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_or(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_max(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_min(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_vectorref(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_vectorset(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_length(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_eval(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_upval(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_sethead(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_settail(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_locals(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_globals(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_list(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_load(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_macro(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_open(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_close(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_map(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_filter(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_let(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_read(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_throw(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_catch(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_writebytes(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_print(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_exit(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_write(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_pack(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_fold(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_no(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_yes(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_unfoldpair(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
int veclisp_n_unfoldvec(struct veclisp_scope *, struct veclisp_cell, struct veclisp_cell *);
#define FORNEXT(var, init) for (var = init; var != NULL; var = var->next)
#define FORPAIR(var, init) for (var = init; var->type == VECLISP_PAIR && var->as.pair != NULL; var = &var->as.pair[1])
#define FORVEC(i, vec) for (i = 1; i <= vec[0].as.integer; ++i)

int main() {
  struct veclisp_scope root_scope;
  struct veclisp_cell last_read, last_eval_result;
  veclisp_init_root_scope(&root_scope);
  for (;;) {
    veclisp_print_prompt(&root_scope);
    if (veclisp_read(&root_scope, &last_read)) {
      if (last_read.type == VECLISP_INT && last_read.as.integer == EOF) break;
      veclisp_print_err(&root_scope, last_read);
    } else if (veclisp_eval(&root_scope, last_read, &last_eval_result)) {
      veclisp_print_err(&root_scope, last_eval_result);
    } else {
      veclisp_write_result(&root_scope, last_eval_result);
    }
  }
}

char *veclisp_intern(char *sym) {
  struct veclisp_interned_syms *s;
  if (veclisp_interned_syms == NULL) {
    veclisp_interned_syms = malloc(sizeof(*veclisp_interned_syms));
    veclisp_interned_syms->sym = sym;
    veclisp_interned_syms->next = NULL;
    return sym;
  }
  FORNEXT(s, veclisp_interned_syms) {
    if (!strcmp(sym, s->sym)) {
      free(sym);
      return s->sym;
    }
  }
  s = malloc(sizeof(*s));
  s->next = veclisp_interned_syms;
  s->sym = sym;
  veclisp_interned_syms = s;
  return sym;
}
void veclisp_print_prompt(struct veclisp_scope *scope) {
  struct veclisp_cell out, prompt;
  if (veclisp_scope_lookup(scope, VECLISP_OUTPORT, &out) || out.type != VECLISP_INT) {
    out.type = VECLISP_INT;
    out.as.integer = (int64_t)stdout;
  }
  if (veclisp_scope_lookup(scope, VECLISP_PROMPT, &prompt) || prompt.type != VECLISP_SYM) {
    prompt.type = VECLISP_SYM;
    prompt.as.sym = VECLISP_DEFAULT_PROMPT;
  }
  fputs(prompt.as.sym, (FILE *)out.as.integer);
}
void veclisp_write(struct veclisp_scope *scope, struct veclisp_cell value) {
  struct veclisp_cell out;
  if (veclisp_scope_lookup(scope, VECLISP_OUTPORT, &out) || out.type != VECLISP_INT) {
    out.type = VECLISP_INT;
    out.as.integer = (int64_t)stdout;
  }
  veclisp_fwrite((FILE *)out.as.integer, value);
}
void veclisp_write_result(struct veclisp_scope *scope, struct veclisp_cell value) {
  struct veclisp_cell out, response;
  if (veclisp_scope_lookup(scope, VECLISP_OUTPORT, &out) || out.type != VECLISP_INT) {
    out.type = VECLISP_INT;
    out.as.integer = (int64_t)stdout;
  }
  if (veclisp_scope_lookup(scope, VECLISP_RESPONSE, &response) || response.type != VECLISP_SYM) {
    response.type = VECLISP_SYM;
    response.as.sym = VECLISP_DEFAULT_RESPONSE;
  }
  fputs(response.as.sym, (FILE *)out.as.integer);
  veclisp_write(scope, value);
  fputc('\n', (FILE *)out.as.integer);
}
void veclisp_print_err(struct veclisp_scope *scope, struct veclisp_cell err) {
  struct veclisp_cell errport;
  if (veclisp_scope_lookup(scope, VECLISP_ERRPORT, &errport) || errport.type != VECLISP_INT) {
    errport.type = VECLISP_INT;
    errport.as.integer = (int64_t)stderr;
  }
  fputs("! ", (FILE *)errport.as.integer);
  veclisp_fwrite((FILE *)errport.as.integer, err);
  fputc('\n', (FILE *)errport.as.integer);
}
int veclisp_init_root_scope(struct veclisp_scope *root_scope) {
  struct veclisp_cell value;
  VECLISP_T = veclisp_intern("t");
  VECLISP_INPORT = veclisp_intern("*In");
  VECLISP_OUTPORT = veclisp_intern("*Out");
  VECLISP_ERRPORT = veclisp_intern("*Err");
  VECLISP_PROMPT = veclisp_intern("*Prompt");
  VECLISP_DEFAULT_PROMPT = veclisp_intern("> ");
  VECLISP_QUOTE = veclisp_intern("quote");
  VECLISP_UNQUOTE = veclisp_intern("unquote");
  VECLISP_RESPONSE = veclisp_intern("*Response");
  VECLISP_DEFAULT_RESPONSE = veclisp_intern("; ");
  VECLISP_ERR_ILLEGAL_DOTTED_LIST = veclisp_intern("illegal dotted list");
  VECLISP_ERR_EXPECTED_CLOSE_PAREN = veclisp_intern("expected closing parentheses");
  VECLISP_ERR_CANNOT_EXEC_VEC = veclisp_intern("cannot execute a vector. expected integer or pair");
  VECLISP_ERR_INVALID_NAME = veclisp_intern("invalid name. expected a symbol");
  VECLISP_ERR_EXPECTED_PAIR = veclisp_intern("invalid value. expected a pair");
  VECLISP_ERR_ILLEGAL_LAMBDA_LIST = veclisp_intern("illegal lambda list");
  VECLISP_ERR_EXPECTED_INT = veclisp_intern("expected an integer");
  VECLISP_ERR_INVALID_SEQUENCE = veclisp_intern("invalid sequence. expected a vector or pair");
  VECLISP_ERR_CANNOT_UPVAL_AT_TOPLEVEL = veclisp_intern("cannot upval at toplevel");
  root_scope->bindings = NULL;
  value.type = VECLISP_INT;
  value.as.integer = (int64_t)stdout;
  veclisp_set(root_scope, VECLISP_OUTPORT, value);
  value.as.integer = (int64_t)stdin;
  veclisp_set(root_scope, VECLISP_INPORT, value);
  value.as.integer = (int64_t)stderr;
  veclisp_set(root_scope, VECLISP_ERRPORT, value);
  value.as.integer = (int64_t)veclisp_n_quote;
  veclisp_set(root_scope, VECLISP_QUOTE, value);
  value.as.integer = (int64_t)veclisp_n_intp;
  veclisp_set(root_scope, veclisp_intern("int?"), value);
  value.as.integer = (int64_t)veclisp_n_symp;
  veclisp_set(root_scope, veclisp_intern("sym?"), value);
  value.as.integer = (int64_t)veclisp_n_vecp;
  veclisp_set(root_scope, veclisp_intern("vec?"), value);
  value.as.integer = (int64_t)veclisp_n_pairp;
  veclisp_set(root_scope, veclisp_intern("pair?"), value);
  value.as.integer = (int64_t)veclisp_n_nilp;
  veclisp_set(root_scope, veclisp_intern("nil?"), value);
  value.as.integer = (int64_t)veclisp_n_pair;
  veclisp_set(root_scope, veclisp_intern("pair"), value);
  value.as.integer = (int64_t)veclisp_n_head;
  veclisp_set(root_scope, veclisp_intern("head"), value);
  value.as.integer = (int64_t)veclisp_n_tail;
  veclisp_set(root_scope, veclisp_intern("tail"), value);
  value.as.integer = (int64_t)veclisp_n_cmp;
  veclisp_set(root_scope, veclisp_intern("<=>"), value);
  value.as.integer = (int64_t)veclisp_n_eq;
  veclisp_set(root_scope, veclisp_intern("="), value);
  value.as.integer = (int64_t)veclisp_n_gt;
  veclisp_set(root_scope, veclisp_intern(">"), value);
  value.as.integer = (int64_t)veclisp_n_lt;
  veclisp_set(root_scope, veclisp_intern("<"), value);
  value.as.integer = (int64_t)veclisp_n_lte;
  veclisp_set(root_scope, veclisp_intern("<="), value);
  value.as.integer = (int64_t)veclisp_n_gte;
  veclisp_set(root_scope, veclisp_intern(">="), value);
  value.as.integer = (int64_t)veclisp_n_set;
  veclisp_set(root_scope, veclisp_intern("set"), value);
  value.as.integer = (int64_t)veclisp_n_syms;
  veclisp_set(root_scope, veclisp_intern("syms"), value);
  value.as.integer = (int64_t)veclisp_n_add;
  veclisp_set(root_scope, veclisp_intern("+"), value);
  value.as.integer = (int64_t)veclisp_n_sub;
  veclisp_set(root_scope, veclisp_intern("-"), value);
  value.as.integer = (int64_t)veclisp_n_mul;
  veclisp_set(root_scope, veclisp_intern("*"), value);
  value.as.integer = (int64_t)veclisp_n_div;
  veclisp_set(root_scope, veclisp_intern("/"), value);
  value.as.integer = (int64_t)veclisp_n_mod;
  veclisp_set(root_scope, veclisp_intern("%"), value);
  value.as.integer = (int64_t)veclisp_n_exp;
  veclisp_set(root_scope, veclisp_intern("exp"), value);
  value.as.integer = (int64_t)veclisp_n_rsh;
  veclisp_set(root_scope, veclisp_intern("bitwise-shift-right"), value);
  value.as.integer = (int64_t)veclisp_n_lsh;
  veclisp_set(root_scope, veclisp_intern("bitwise-shift-left"), value);
  value.as.integer = (int64_t)veclisp_n_bitwiseand;
  veclisp_set(root_scope, veclisp_intern("bitwise-and"), value);
  value.as.integer = (int64_t)veclisp_n_bitwiseor;
  veclisp_set(root_scope, veclisp_intern("bitwise-or"), value);
  value.as.integer = (int64_t)veclisp_n_bitwisexor;
  veclisp_set(root_scope, veclisp_intern("bitwise-xor"), value);
  value.as.integer = (int64_t)veclisp_n_bitwisenot;
  veclisp_set(root_scope, veclisp_intern("bitwise-not"), value);
  value.as.integer = (int64_t)veclisp_n_abs;
  veclisp_set(root_scope, veclisp_intern("abs"), value);
  value.as.integer = (int64_t)veclisp_n_sqrt;
  veclisp_set(root_scope, veclisp_intern("sqrt"), value);
  value.as.integer = (int64_t)veclisp_n_rand;
  veclisp_set(root_scope, veclisp_intern("rand"), value);
  value.as.integer = (int64_t)veclisp_n_max;
  veclisp_set(root_scope, veclisp_intern("max"), value);
  value.as.integer = (int64_t)veclisp_n_min;
  veclisp_set(root_scope, veclisp_intern("min"), value);
  value.as.integer = (int64_t)veclisp_n_length;
  veclisp_set(root_scope, veclisp_intern("length"), value);
  value.as.integer = (int64_t)veclisp_n_and;
  veclisp_set(root_scope, veclisp_intern("and"), value);
  value.as.integer = (int64_t)veclisp_n_or;
  veclisp_set(root_scope, veclisp_intern("or"), value);
  value.as.integer = (int64_t)veclisp_n_vectorref;
  veclisp_set(root_scope, veclisp_intern("vector-ref"), value);
  value.as.integer = (int64_t)veclisp_n_vectorset;
  veclisp_set(root_scope, veclisp_intern("vector-set"), value);
  value.as.integer = (int64_t)veclisp_n_eval;
  veclisp_set(root_scope, veclisp_intern("eval"), value);
  value.as.integer = (int64_t)veclisp_n_upval;
  veclisp_set(root_scope, veclisp_intern("upval"), value);
  value.as.integer = (int64_t)veclisp_n_sethead;
  veclisp_set(root_scope, veclisp_intern("set-head"), value);
  value.as.integer = (int64_t)veclisp_n_settail;
  veclisp_set(root_scope, veclisp_intern("set-tail"), value);
  value.as.integer = (int64_t)veclisp_n_locals;
  veclisp_set(root_scope, veclisp_intern("locals"), value);
  value.as.integer = (int64_t)veclisp_n_globals;
  veclisp_set(root_scope, veclisp_intern("globals"), value);
  value.as.integer = (int64_t)veclisp_n_list;
  veclisp_set(root_scope, veclisp_intern("list"), value);
  value.as.integer = (int64_t)veclisp_n_load;
  veclisp_set(root_scope, veclisp_intern("load"), value);
  value.as.integer = (int64_t)veclisp_n_macro;
  veclisp_set(root_scope, veclisp_intern("macro"), value);
  value.as.integer = (int64_t)veclisp_n_open;
  veclisp_set(root_scope, veclisp_intern("open"), value);
  value.as.integer = (int64_t)veclisp_n_close;
  veclisp_set(root_scope, veclisp_intern("close"), value);
  value.as.integer = (int64_t)veclisp_n_map;
  veclisp_set(root_scope, veclisp_intern("map"), value);
  value.as.integer = (int64_t)veclisp_n_filter;
  veclisp_set(root_scope, veclisp_intern("filter"), value);
  value.as.integer = (int64_t)veclisp_n_let;
  veclisp_set(root_scope, veclisp_intern("let"), value);
  value.as.integer = (int64_t)veclisp_n_read;
  veclisp_set(root_scope, veclisp_intern("read"), value);
  value.as.integer = (int64_t)veclisp_n_catch;
  veclisp_set(root_scope, veclisp_intern("catch"), value);
  value.as.integer = (int64_t)veclisp_n_throw;
  veclisp_set(root_scope, veclisp_intern("throw"), value);
  value.as.integer = (int64_t)veclisp_n_writebytes;
  veclisp_set(root_scope, veclisp_intern("write-bytes"), value);
  value.as.integer = (int64_t)veclisp_n_print;
  veclisp_set(root_scope, veclisp_intern("print"), value);
  value.as.integer = (int64_t)veclisp_n_exit;
  veclisp_set(root_scope, veclisp_intern("exit"), value);
  value.as.integer = (int64_t)veclisp_n_write;
  veclisp_set(root_scope, veclisp_intern("write"), value);
  value.as.integer = (int64_t)veclisp_n_pack;
  veclisp_set(root_scope, veclisp_intern("pack"), value);
  value.as.integer = (int64_t)veclisp_n_fold;
  veclisp_set(root_scope, veclisp_intern("fold"), value);
  value.as.integer = (int64_t)veclisp_n_unfoldpair;
  veclisp_set(root_scope, veclisp_intern("unfold-pair"), value);
  value.as.integer = (int64_t)veclisp_n_unfoldvec;
  veclisp_set(root_scope, veclisp_intern("unfold-vec"), value);
  value.as.integer = (int64_t)veclisp_n_yes;
  veclisp_set(root_scope, veclisp_intern("yes"), value);
  value.as.integer = (int64_t)veclisp_n_no;
  veclisp_set(root_scope, veclisp_intern("no"), value);
  value.type = VECLISP_SYM;
  value.as.sym = VECLISP_DEFAULT_PROMPT;
  veclisp_set(root_scope, VECLISP_PROMPT, value);
  value.as.sym = VECLISP_DEFAULT_RESPONSE;
  veclisp_set(root_scope, VECLISP_RESPONSE, value);
  return 0;
}
int skip_space(FILE *in) {
  int c;
  do { c = fgetc(in); }
  while (isspace(c));
  return c;
}
struct veclisp_cell *veclisp_alloc_pair() {
  return GC_malloc(sizeof(struct veclisp_cell) * 2);
}
int veclisp_read(struct veclisp_scope *scope, struct veclisp_cell *result) {
  int c, sign = 1, buf_allocated, buf_used;
  char *sym_buf;
  struct veclisp_cell inport, *p;
  if (veclisp_scope_lookup(scope, VECLISP_INPORT, &inport) || inport.type != VECLISP_INT) {
    inport.type = VECLISP_INT;
    inport.as.integer = (int64_t)stdin;
  }
  c = skip_space((FILE *)inport.as.integer);
  if (c == EOF) {
    result->type = VECLISP_INT;
    result->as.integer = EOF;
    return 1;
  }
  if (c == '(') {
    result->type = VECLISP_PAIR;
    c = skip_space((FILE *)inport.as.integer);
    if (c == ')') {
      result->as.pair = NULL;
      return 0;
    }
    result->as.pair = veclisp_alloc_pair();
    ungetc(c, (FILE *)inport.as.integer);
    for (p = result->as.pair; p != NULL; p[1].as.pair = veclisp_alloc_pair(), p = p[1].as.pair) {
      if (veclisp_read(scope, p)) {
        *result = *p;
        return 1;
      }
      p[1].type = VECLISP_PAIR;
      c = skip_space((FILE *)inport.as.integer);
      if (c == ')') {
        p[1].as.pair = NULL;
        return 0;
      } else if (c == '.') {
        if (veclisp_read(scope, &p[1])) return 1;
        c = skip_space((FILE *)inport.as.integer);
        if (c != ')') {
          result->type = VECLISP_SYM;
          result->as.sym = VECLISP_ERR_ILLEGAL_DOTTED_LIST;
          return 1;
        }
        return 0;
      }
      ungetc(c, (FILE *)inport.as.integer);
    }
  } else if (c == '[') {
    buf_used = 1;
    buf_allocated = 16;
    result->type = VECLISP_VEC;
    result->as.vec = GC_malloc(sizeof(*result->as.vec) * buf_allocated);
    result->as.vec[0].type = VECLISP_INT;
    do {
      if (buf_used >= buf_allocated) {
        buf_allocated *= 2;
        result->as.vec = GC_realloc(result->as.vec, sizeof(*result->as.vec) * buf_allocated);
      }
      c = skip_space((FILE *)inport.as.integer);
      if (c == ']') {
        result->as.vec = GC_realloc(result->as.vec, sizeof(*result->as.vec) * buf_used);
        result->as.vec[0].as.integer = buf_used - 1;
        return 0;
      }
      ungetc(c, (FILE *)inport.as.integer);
    } while (!veclisp_read(scope, &result->as.vec[buf_used++]));
  } else if (c == '-') {
    sign = -1;
    c = fgetc((FILE *)inport.as.integer);
    if (isdigit(c)) goto read_int;
    ungetc(c, (FILE *)inport.as.integer);
    c = '-';
    goto read_symbol;
  } else if (isdigit(c)) {
  read_int:
    result->type = VECLISP_INT;
    result->as.integer = 0;
    do {
      result->as.integer *= 10;
      result->as.integer += c - '0';
      c = fgetc((FILE *)inport.as.integer);
    } while (isdigit(c));
    result->as.integer *= sign;
    ungetc(c, (FILE *)inport.as.integer);
    return 0;
  } else if (c == ')' || c == ']') {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_EXPECTED_CLOSE_PAREN;
    return 1;
  } else if (c == '"') {
    result->type = VECLISP_SYM;
    buf_allocated = 32;
    buf_used = 0;
    sym_buf = malloc(sizeof(*sym_buf) * buf_allocated);
    for (;;) {
      c = fgetc((FILE *)inport.as.integer);
      if (buf_used >= buf_allocated) {
        buf_allocated *= 2;
        sym_buf = realloc(sym_buf, sizeof(*sym_buf) *buf_allocated);
      }
      if (c == '\\') {
        c = fgetc((FILE *)inport.as.integer);
      } else if (c == '"') break;
      sym_buf[buf_used++] = c;
    }
    sym_buf[buf_used++] = 0;
    sym_buf = realloc(sym_buf, sizeof(*sym_buf) * buf_used);
    result->as.sym = veclisp_intern(sym_buf);
  } else if (c == '\'') {
    result->type = VECLISP_PAIR;
    result->as.pair = veclisp_alloc_pair();
    result->as.pair[0].type = VECLISP_SYM;
    result->as.pair[0].as.sym = VECLISP_QUOTE;
    return veclisp_read(scope, &result->as.pair[1]);
  } else if (c == ',') {
    result->type = VECLISP_PAIR;
    result->as.pair = veclisp_alloc_pair();
    result->as.pair[0].type = VECLISP_SYM;
    result->as.pair[0].as.sym = VECLISP_UNQUOTE;
    return veclisp_read(scope, &result->as.pair[1]);
  } else {
  read_symbol:
    result->type = VECLISP_SYM;
    buf_allocated = 32;
    buf_used = 0;
    sym_buf = malloc(sizeof(*sym_buf) * buf_allocated);
    sym_buf[buf_used++] = c;
    for (;;) {
      c = fgetc((FILE *)inport.as.integer);
      if (buf_used >= buf_allocated) {
        buf_allocated *= 2;
        sym_buf = realloc(sym_buf, sizeof(*sym_buf) * buf_allocated);
      }
      if (isspace(c) || c == ')' || c == ']' || c == '[' || c == '(') {
        ungetc(c, (FILE *)inport.as.integer);
        break;
      }
      sym_buf[buf_used++] = c;
    }
    sym_buf[buf_used++] = 0;
    sym_buf = realloc(sym_buf, sizeof(*sym_buf) * buf_used);
    result->as.sym = veclisp_intern(sym_buf);
    return 0;
  }
  return 0;
}
int veclisp_eval(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  int64_t i;
  switch (value.type) {
  case VECLISP_INT:
    *result = value;
    return 0;
  case VECLISP_SYM:
    veclisp_scope_lookup(scope, value.as.sym, result);
    return 0;
  case VECLISP_VEC:
    result->type = VECLISP_VEC;
    result->as.vec = GC_malloc(sizeof(struct veclisp_cell) * (value.as.vec[0].as.integer + 1));
    result->as.vec[0] = value.as.vec[0];
    for (i = 1; i <= value.as.vec[0].as.integer; ++i)
      if (veclisp_eval(scope, value.as.vec[i], &result->as.vec[i])) return 1;
    return 0;
  case VECLISP_PAIR:
    if (value.as.pair == NULL) {
      *result = value;
      return 0;
    }
    return veclisp_n_call(scope, value, result);
  default:
    return 1;
  }
}
int veclisp_scope_lookup(struct veclisp_scope *scope, char *sym, struct veclisp_cell *result) {
  struct veclisp_scope *s;
  struct veclisp_bindings *b;
  FORNEXT(s, scope) {
    FORNEXT(b, s->bindings) {
      if (b->sym == sym) {
        *result = b->value;
        return 0;
      }
    }
  }
  result->type = VECLISP_PAIR;
  result->as.pair = NULL;
  return 1;
}
void veclisp_set(struct veclisp_scope *scope, char *interned_sym, struct veclisp_cell value) {
  struct veclisp_scope *s = NULL;
  struct veclisp_bindings *b = NULL;
  FORNEXT(s, scope) {
    if (s->bindings) {
      FORNEXT(b, s->bindings) {
        if (b->sym == interned_sym) {
          b->value = value;
          return;
        }
      }
    }
    if (!s->next) {
      b = GC_malloc(sizeof(*b));
      b->next = s->bindings;
      b->sym = interned_sym;
      b->value = value;
      s->bindings = b;
    }
  }
}
int veclisp_contains_special_chars(char *sym) {
  for (int i = 0; sym[i] != 0; ++i) {
    if (isspace(sym[i])) return 1;
    switch (sym[i]) {
    case '(':
    case ')':
    case '[':
    case ']':
    case '"':
      return 1;
    case '.':
    case '\'':
    case ',':
      if (i == 0) return 1;
    }
  }
  return 0;
}
void veclisp_fwrite(FILE *out, struct veclisp_cell value) {
  int64_t len, i;
  struct veclisp_cell *s;
  switch (value.type) {
  case VECLISP_INT:
    fprintf(out, "%li", value.as.integer);
    break;
  case VECLISP_SYM:
    if (veclisp_contains_special_chars(value.as.sym)) {
      fputc('"', out);
      for (i = 0; value.as.sym[i] != 0; i+=len) {
        for (len = 1; value.as.sym[i+len] != '\\' && value.as.sym[i+len] != '"'; ++len) {
          if (value.as.sym[i+len] == 0) {
            fprintf(out, "%s\"", value.as.sym + i);
            return;
          }
        }
        fprintf(out, "%*s\\", (int)len, value.as.sym + i);
      }
    } else {
      fputs(value.as.sym, out);
    }
    break;
  case VECLISP_PAIR:
    fputc('(', out);
    for (s = value.as.pair; s != NULL; s = s[1].as.pair) {
      veclisp_fwrite(out, s[0]);
      if (s[1].type != VECLISP_PAIR) {
        fputs(" . ", out);
        veclisp_fwrite(out, s[1]);
        break;
      }
      if (s[1].as.pair != NULL) {
        fputc(' ', out);
      }
    }
    fputc(')', out);
    break;
  case VECLISP_VEC:
    len = (int64_t)value.as.vec[0].as.integer;
    if (len < 0) {
      fprintf(out, "(INVALID VECTOR LEN %li)", len);
    } else {
      fputc('[', out);
      for (i = 1; i <= len; ++i) {
        veclisp_fwrite(out, value.as.vec[i]);
        if (i != len) fputc(' ', out);
      }
      fputc(']', out);
    }
    break;
  }
}
int veclisp_n_quote(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  *result = value;
  return 0;
}
int veclisp_n_intp(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type == VECLISP_INT) {
    *result = value;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_symp(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type == VECLISP_SYM) {
    *result = value;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_pairp(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type == VECLISP_PAIR) {
    *result = value;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_vecp(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type == VECLISP_VEC) {
    *result = value;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_nilp(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type == VECLISP_PAIR && value.as.pair == NULL) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_T;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_pair(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  result->type = VECLISP_PAIR;
  result->as.pair = veclisp_alloc_pair();
  return veclisp_eval(scope, value.as.pair[0], &result->as.pair[0])
    || veclisp_eval(scope, value.as.pair[1].as.pair[0], &result->as.pair[1]);
}
int veclisp_n_head(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type != VECLISP_PAIR || value.as.pair == NULL) {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  } else {
    *result = value.as.pair[0];
  }
  return 0;
}
int veclisp_n_tail(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result) {
  if (veclisp_eval(scope, value.as.pair[0], &value)) return 1;
  if (value.type != VECLISP_PAIR || value.as.pair == NULL) {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  } else {
    *result = value.as.pair[1];
  }
  return 0;
}
int veclisp_compare(struct veclisp_cell x, struct veclisp_cell y) {
  int i, r;
  if (x.type == y.type) {
    switch (y.type) {
    case VECLISP_INT:
      if (x.as.integer > y.as.integer) return 1;
      else if (x.as.integer < y.as.integer) return -1;
      else return 0;
    case VECLISP_SYM:
      if (x.as.sym == y.as.sym) return 0;
      else return strcmp(x.as.sym, y.as.sym);
    case VECLISP_VEC:
      if (x.as.vec == y.as.vec) return 0;
      if (x.as.vec[0].as.integer > y.as.vec[0].as.integer) return 1;
      if (x.as.vec[0].as.integer < y.as.vec[0].as.integer) return 1;
      for (i = 1; i <= x.as.vec[0].as.integer; ++i) {
        r = veclisp_compare(x.as.vec[i], y.as.vec[i]);
        if (r != 0) return r;
      }
      return 0;
    case VECLISP_PAIR:
      if (x.as.pair == y.as.pair) return 0;
      r = veclisp_compare(x.as.pair[0], y.as.pair[0]);
      if (r != 0) return r;
      return veclisp_compare(x.as.pair[1], y.as.pair[1]);
    default: return 0;
    }
  } else if (x.type == VECLISP_PAIR && x.as.pair == NULL) return -1;
  else if (y.type == VECLISP_PAIR && y.as.pair == NULL) return 1;
  else if (x.type > y.type) return 1;
  else return -1;
}
int veclisp_n_cmp(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x, y;
  result->type = VECLISP_INT;
  if (veclisp_eval(scope, args.as.pair[0], &x)) return 1;
  FORPAIR(a, &args.as.pair[1]) {
    if (veclisp_eval(scope, a->as.pair[0], &y)) return 1;
    result->as.integer = veclisp_compare(x, y);
    if (result->as.integer != 0) break;
    x = y;
  }
  return 0;
}
int veclisp_n_eq(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_n_cmp(scope, args, result)) return 1;
  if (result->as.integer == 0) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_T;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_gt(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_n_cmp(scope, args, result)) return 1;
  if (result->as.integer > 0) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_T;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_lt(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_n_cmp(scope, args, result)) return 1;
  if (result->as.integer < 0) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_T;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_gte(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_n_cmp(scope, args, result)) return 1;
  if (result->as.integer >= 0) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_T;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_lte(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_n_cmp(scope, args, result)) return 1;
  if (result->as.integer <= 0) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_T;
  } else {
    result->type = VECLISP_PAIR;
    result->as.pair = NULL;
  }
  return 0;
}
int veclisp_n_set(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell name;
  if (veclisp_eval(scope, args.as.pair[0], &name)) return 1;
  if (name.type != VECLISP_SYM) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_INVALID_NAME;
    return 1;
  }
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], result)) return 1;
  veclisp_set(scope, name.as.sym, *result);
  return 0;
}
int veclisp_n_syms(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_scope *s;
  struct veclisp_bindings *b;
  struct veclisp_cell *r;
  result->type = VECLISP_PAIR;
  result->as.pair = veclisp_alloc_pair();
  r = result;
  FORNEXT(s, scope) {
    if (s->bindings) {
      FORNEXT(b, s->bindings) {
        r->as.pair[0].type = VECLISP_SYM;
        r->as.pair[0].as.sym = b->sym;
        r->as.pair[1].type = VECLISP_PAIR;
        if (b->next == NULL && s->next == NULL) {
          r->as.pair[1].as.pair = NULL;
          return 0;
        } else {
          r->as.pair[1].as.pair = veclisp_alloc_pair();
          r = &r->as.pair[1];
        }
      }
    }
  }
  return 0;
}
int veclisp_n_add(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    result->as.integer += x.as.integer;
  }
  return 0;
}
int veclisp_n_sub(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    result->as.integer -= x.as.integer;
  }
  return 0;
}
int veclisp_n_mul(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 1;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    result->as.integer *= x.as.integer;
  }
  return 0;
}
int veclisp_n_div(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer /= x.as.integer;
  }
  return 0;
}
int veclisp_n_mod(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer %= x.as.integer;
  }
  return 0;
}
int veclisp_n_exp(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer = (int64_t)powl(result->as.integer, x.as.integer);
  }
  return 0;
}
int veclisp_n_rsh(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer >>= x.as.integer;
  }
  return 0;
}
int veclisp_n_lsh(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer <<= x.as.integer;
  }
  return 0;
}
int veclisp_n_bitwiseand(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer &= x.as.integer;
  }
  return 0;
}
int veclisp_n_bitwiseor(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer |= x.as.integer;
  }
  return 0;
}
int veclisp_n_bitwisexor(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &x)) return 1;
    if (a == &args) result->as.integer = x.as.integer;
    else result->as.integer ^= x.as.integer;
  }
  return 0;
}
int veclisp_n_bitwisenot(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  result->type = VECLISP_INT;
  result->as.integer = ~result->as.integer;
  return 0;
}
int veclisp_n_abs(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  result->type = VECLISP_INT;
  result->as.integer = abs(result->as.integer);
  return 0;
}
int veclisp_n_sqrt(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  result->type = VECLISP_INT;
  result->as.integer = (int64_t)sqrtl(result->as.integer);
  return 0;
}
int veclisp_n_rand(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  unsigned int seed;
  if (args.as.pair != NULL) {
    if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
    seed = (unsigned int)result->as.integer;
    result->type = VECLISP_PAIR;
    result->as.pair = veclisp_alloc_pair();
    result->as.pair[0].type = VECLISP_INT;
    result->as.pair[0].as.integer = (int64_t)rand_r(&seed);
    result->as.pair[1].type = VECLISP_INT;
    result->as.pair[1].as.integer = (int64_t)seed;
    return 0;
  }
  result->type = VECLISP_INT;
  result->as.integer = (int64_t)rand();
  return 0;
}
int veclisp_n_max(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x, y;
  if (veclisp_eval(scope, args.as.pair[0], &x)) return 1;
  FORPAIR(a, &args.as.pair[1]) {
    if (veclisp_eval(scope, a->as.pair[0], &y)) return 1;
    if (veclisp_compare(x, y) > 0) x = y;
  }
  *result = x;
  return 0;
}
int veclisp_n_min(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, x, y;
  if (veclisp_eval(scope, args.as.pair[0], &x)) return 1;
  FORPAIR(a, &args.as.pair[1]) {
    if (veclisp_eval(scope, a->as.pair[0], &y)) return 1;
    if (veclisp_compare(x, y) < 0) x = y;
  }
  *result = x;
  return 0;
}
int veclisp_n_length(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, l;
  if (veclisp_eval(scope, args.as.pair[0], &l)) return 1;
  result->type = VECLISP_INT;
  result->as.integer = 0;
  switch (l.type) {
  case VECLISP_PAIR:
    FORPAIR(a, &l) result->as.integer++;
    break;
  case VECLISP_VEC:
    result->as.integer = l.as.vec[0].as.integer;
    break;
  case VECLISP_SYM:
    result->as.integer = strlen(l.as.sym);
  default: break;
  }
  return 0;
}
int veclisp_n_vectorref(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell v, i;
  if (veclisp_eval(scope, args.as.pair[0], &v)) return 1;
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], &i)) return 1;
  *result = v.as.vec[1 + i.as.integer];
  return 0;
}
int veclisp_n_vectorset(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell v, i, x;
  if (veclisp_eval(scope, args.as.pair[0], &v)) return 1;
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], &i)) return 1;
  if (veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[0], &x)) return 1;
  v.as.vec[1 + i.as.integer] = x;
  *result = x;
  return 0;
}
int veclisp_n_and(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], result)) return 1;
    if (result->type == VECLISP_PAIR && result->as.pair == NULL) break;
  }
  return 0;
}
int veclisp_n_or(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], result)) return 1;
    if (result->type != VECLISP_PAIR || result->as.pair != NULL) break;
  }
  return 0;
}
int veclisp_n_eval(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  return veclisp_eval(scope, *result, result);
}
int veclisp_n_upval(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (!scope->next) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_CANNOT_UPVAL_AT_TOPLEVEL;
    return 1;
  }
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  return veclisp_eval(scope->next, *result, result);
}
int veclisp_n_sethead(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell pair;
  if (veclisp_eval(scope, args.as.pair[0], &pair)) return 1;
  if (pair.type != VECLISP_PAIR) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_EXPECTED_PAIR;
    return 1;
  }
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], result)) return 1;
  pair.as.pair[0] = *result;
  return 0;
}
int veclisp_n_settail(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell pair;
  if (veclisp_eval(scope, args.as.pair[0], &pair)) return 1;
  if (pair.type != VECLISP_PAIR) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_EXPECTED_PAIR;
    return 1;
  }
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], result)) return 1;
  pair.as.pair[1] = *result;
  return 0;
}
int veclisp_n_locals(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *r;
  struct veclisp_bindings *b;
  result->type = VECLISP_PAIR;
  if (!scope->bindings) {
    result->as.pair = NULL;
    return 0;
  }
  result->as.pair = veclisp_alloc_pair();
  r = result;
  FORNEXT(b, scope->bindings) {
    r->as.pair[0].type = VECLISP_SYM;
    r->as.pair[0].as.sym = b->sym;
    r->as.pair[1].type = VECLISP_PAIR;
    if (b->next) {
      r->as.pair[1].as.pair = veclisp_alloc_pair();
      r = &r->as.pair[1];
    } else {
      r->as.pair[1].as.pair = NULL;
    }
  }
  return 0;
}
int veclisp_n_globals(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_scope *s;
  FORNEXT(s, scope) if (!s->next) return veclisp_n_locals(s, args, result);
  result->type = VECLISP_PAIR;
  result->as.pair = NULL;
  return 0;
}
int veclisp_n_list(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *r, *a;
  result->type = VECLISP_PAIR;
  if (args.as.pair == NULL) {
    result->as.pair = NULL;
    return 0;
  }
  result->as.pair = veclisp_alloc_pair();
  r = result;
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &r->as.pair[0])) return 1;
    r->as.pair[1].type = VECLISP_PAIR;
    if (a->as.pair[1].as.pair != NULL) {
      r->as.pair[1].as.pair = veclisp_alloc_pair();
      r = &r->as.pair[1];
    } else {
      r->as.pair[1].as.pair = NULL;
    }
  }
  return 0;
}
int veclisp_n_call(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell lambda_head, lambda_tail, *t, *a;
  if (veclisp_eval(scope, args.as.pair[0], &lambda_head)) return 1;
  if (lambda_head.type == VECLISP_PAIR && lambda_head.as.pair == NULL) {
    *result = args.as.pair[0];
    return 1;
  }
  lambda_tail = args.as.pair[1];
  if (lambda_head.type == VECLISP_PAIR && lambda_head.as.pair[0].type == VECLISP_PAIR) {
    if (lambda_tail.type == VECLISP_PAIR) {
      if (lambda_tail.as.pair != NULL) {
        lambda_tail.as.pair = veclisp_alloc_pair();
        t = &lambda_tail;
        FORPAIR(a, &args.as.pair[1]) {
          if (veclisp_eval(scope, a->as.pair[0], &t->as.pair[0])) return 1;
          t->as.pair[1].type = VECLISP_PAIR;
          if (a->as.pair[1].type != VECLISP_PAIR) {
            if (veclisp_eval(scope, a->as.pair[1], &t->as.pair[1])) return 1;
          } else if (a->as.pair[1].as.pair != NULL) {
            t = &t->as.pair[1];
          } else {
            t->as.pair[1].as.pair = NULL;
          }
        }
      }
    } else if (veclisp_eval(scope, lambda_tail, &lambda_tail)) return 1;
  }
  return veclisp_lambda(scope, lambda_head, lambda_tail, result);
}
int veclisp_lambda(struct veclisp_scope *parent_scope, struct veclisp_cell lambda, struct veclisp_cell args, struct veclisp_cell *result) {
  int64_t i;
  struct veclisp_cell *a, *p;
  struct veclisp_scope scope;
  struct veclisp_bindings bindings, *b;
 retry:
  switch (lambda.type) {
  case VECLISP_VEC:
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_CANNOT_EXEC_VEC;
    return 1;
  case VECLISP_SYM:
    if (veclisp_eval(parent_scope, lambda, &lambda)) return 1;
    goto retry;
  case VECLISP_INT:
    return ((veclisp_native_func)lambda.as.integer)(parent_scope, args, result);
  case VECLISP_PAIR:
    if (lambda.as.pair == NULL) {
      result->type = VECLISP_SYM;
      result->as.sym = VECLISP_ERR_ILLEGAL_LAMBDA_LIST;
      return 1;
    }
  }
  scope.bindings = NULL;
  switch (lambda.as.pair[0].type) {
  case VECLISP_SYM:
    scope.bindings = &bindings;
    bindings.sym = lambda.as.pair[0].as.sym;
    bindings.value = args;
    bindings.next = NULL;
    break;
  case VECLISP_PAIR:
    a = &args;
    if (lambda.as.pair[0].as.pair == NULL) {
      result->type = VECLISP_SYM;
      result->as.sym = VECLISP_ERR_ILLEGAL_LAMBDA_LIST;
      return 1;
    }
    FORPAIR(p, &lambda.as.pair[0]) {
      if (!scope.bindings) {
        scope.bindings = &bindings;
        b = &bindings;
      }
      if (p->as.pair[0].type != VECLISP_SYM) {
        result->type = VECLISP_SYM;
        result->as.sym = VECLISP_ERR_INVALID_NAME;
        return 1;
      }
      b->sym = p->as.pair[0].as.sym;
      if (a->type == VECLISP_PAIR) {
        if (a->as.pair == NULL) {
          b->value.type = VECLISP_PAIR;
          b->value.as.pair = NULL;
        } else {
          b->value = a->as.pair[0];
          a = &a->as.pair[1];
        }
      } else {
        b->value = *a;
      }
      if (p->as.pair[1].as.pair == NULL) {
        b->next = NULL;
      } else {
        b->next = GC_malloc(sizeof(*b->next));
        b = b->next;
      }
    }
    break;
  case VECLISP_VEC:
    a = &args;
    FORVEC(i, lambda.as.pair[0].as.vec) {
      if (!scope.bindings) {
        scope.bindings = &bindings;
        b = &bindings;
      }
      if (lambda.as.pair[0].as.vec[i].type != VECLISP_SYM) {
        result->type = VECLISP_SYM;
        result->as.sym = VECLISP_ERR_INVALID_NAME;
        return 1;
      }
      b->sym = lambda.as.pair[0].as.vec[i].as.sym;
      if (a->as.pair == NULL) {
        b->value.type = VECLISP_PAIR;
        b->value.as.pair = NULL;
      } else {
        b->value = a->as.pair[0];
        a = &a->as.pair[1];
      }
      if (i == lambda.as.pair[0].as.vec[0].as.integer) {
        b->next = NULL;
      } else {
        b->next = GC_malloc(sizeof(*b->next));
        b = b->next;
      }
    }
    break;
  default:
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_ILLEGAL_LAMBDA_LIST;
    return 1;
  }
  scope.next = parent_scope;
  FORPAIR(p, &lambda.as.pair[1]) {
    if (veclisp_eval(&scope, p->as.pair[0], result)) return 1;
  }
  return 0;
}
int veclisp_n_load(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  FILE *in;
  struct veclisp_cell infile, last_read;
  struct veclisp_scope load_scope;
  struct veclisp_bindings load_bindings;
  if (veclisp_eval(scope, args.as.pair[0], &infile)) return 1;
  if (infile.type != VECLISP_SYM) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_INVALID_NAME;
    return 1;
  }
  load_scope.next = scope;
  load_scope.bindings = &load_bindings;
  load_bindings.next = NULL;
  load_bindings.sym = VECLISP_INPORT;
  load_bindings.value.type = VECLISP_INT;
  load_bindings.value.as.integer = (int64_t)(in = fopen(infile.as.sym, "r"));
  for (;;) {
    if (veclisp_read(&load_scope, &last_read)) {
      if (last_read.type == VECLISP_INT && last_read.as.integer == EOF) break;
      *result = last_read;
      return 1;
    } else if (veclisp_eval(&load_scope, last_read, result)) return 1;
  }
  fclose(in);
  return 0;
}
int veclisp_n_macro(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, *l, list;
  list.type = VECLISP_PAIR;
  list.as.pair = veclisp_alloc_pair();
  l = &list;
  FORPAIR(a, &args) {
    if (l == &list) l->as.pair[0] = a->as.pair[0];
    else if (veclisp_eval(scope, a->as.pair[0], &l->as.pair[0])) return 1;
    l->as.pair[1].type = VECLISP_PAIR;
    if (a->as.pair[1].as.pair == NULL) l->as.pair[1].as.pair = NULL;
    else {
      l->as.pair[1].as.pair = veclisp_alloc_pair();
      l = &l->as.pair[1];
    }
  }
  return veclisp_eval(scope, list, result);
}
int veclisp_n_open(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell filename, mode;
  if (veclisp_eval(scope, args.as.pair[0], &filename)) return 1;
  if (filename.type != VECLISP_SYM) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_INVALID_NAME;
    return 1;
  }
  if (args.as.pair[1].as.pair == NULL) {
    mode.as.sym = "r";
  } else {
    if (veclisp_eval(scope, args.as.pair[1].as.pair[0], &mode)) return 1;
    if (mode.type != VECLISP_SYM) {
      result->type = VECLISP_SYM;
      result->as.sym = VECLISP_ERR_INVALID_NAME;
      return 1;
    }
  }
  result->type = VECLISP_INT;
  result->as.integer = (int64_t)fopen(filename.as.sym, mode.as.sym);
  return 0;
}
int veclisp_n_close(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  if (result->type != VECLISP_INT) {
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_EXPECTED_INT;
    return 1;
  }
  fclose((FILE *)result->as.integer);
  result->type = VECLISP_PAIR;
  result->as.pair = NULL;
  return 0;
}
int veclisp_n_map(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  int64_t i;
  struct veclisp_cell fun, fun_args, seq, *r, *s;
  if (veclisp_eval(scope, args.as.pair[0], &fun)) return 1;
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], &seq)) return 1;
  switch (seq.type) {
  case VECLISP_VEC:
    result->type = VECLISP_VEC;
    result->as.vec = GC_malloc(sizeof(*result->as.vec) * (1 + seq.as.vec[0].as.integer));
    result->as.vec[0] = seq.as.vec[0];
    FORVEC(i, seq.as.vec) {
      fun_args.type = VECLISP_PAIR;
      fun_args.as.pair = veclisp_alloc_pair();
      fun_args.as.pair[0] = seq.as.vec[i];
      fun_args.as.pair[1].type = VECLISP_PAIR;
      fun_args.as.pair[1].as.pair = NULL;
      if (veclisp_lambda(scope, fun, fun_args, &result->as.vec[i])) return 1;
    }
    return 0;
  case VECLISP_PAIR:
    result->type = VECLISP_PAIR;
    if (seq.as.pair == NULL) {
      result->as.pair = NULL;
      return 0;
    }
    result->as.pair = veclisp_alloc_pair();
    r = result;
    FORPAIR(s, &seq) {
      fun_args.type = VECLISP_PAIR;
      fun_args.as.pair = veclisp_alloc_pair();
      fun_args.as.pair[0] = s->as.pair[0];
      fun_args.as.pair[1].type = VECLISP_PAIR;
      fun_args.as.pair[1].as.pair = NULL;
      if (veclisp_lambda(scope, fun, fun_args, &r->as.pair[0])) return 1;
      r->as.pair[1].type = VECLISP_PAIR;
      if (s->as.pair[1].type != VECLISP_PAIR) {
        fun_args.as.pair = veclisp_alloc_pair();
        fun_args.as.pair[0] = s->as.pair[1];
        fun_args.as.pair[1].type = VECLISP_PAIR;
        fun_args.as.pair[1].as.pair = NULL;
        return veclisp_lambda(scope, fun, fun_args, &r->as.pair[1]);
      } else if (s->as.pair[1].as.pair == NULL) {
        r->as.pair[1].as.pair = NULL;
      } else {
        r->as.pair[1].as.pair = veclisp_alloc_pair();
        r = &r->as.pair[1];
      }
    }
    return 0;
  default:
  case VECLISP_INT:
  case VECLISP_SYM:
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_INVALID_SEQUENCE;
    return 1;
  }
}
int veclisp_n_filter(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  int64_t i, vec_allocated, vec_used;
  struct veclisp_cell fun, fun_args, seq, *r, *s, t;
  if (veclisp_eval(scope, args.as.pair[0], &fun)) return 1;
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], &seq)) return 1;
  switch (seq.type) {
  case VECLISP_VEC:
    vec_allocated = 1 + seq.as.vec[0].as.integer;
    vec_used = 0;
    result->type = VECLISP_VEC;
    result->as.vec = GC_malloc(sizeof(*result->as.vec) * vec_allocated);
    result->as.vec[vec_used++] = seq.as.vec[0];
    FORVEC(i, seq.as.vec) {
      fun_args.type = VECLISP_PAIR;
      fun_args.as.pair = veclisp_alloc_pair();
      fun_args.as.pair[0] = seq.as.vec[i];
      fun_args.as.pair[1].type = VECLISP_PAIR;
      fun_args.as.pair[1].as.pair = NULL;
      if (veclisp_lambda(scope, fun, fun_args, &result->as.vec[vec_used])) return 1;
      if (result->as.vec[vec_used].type != VECLISP_PAIR || result->as.vec[vec_used].as.pair != NULL) {
        result->as.vec[vec_used++] = seq.as.vec[i];
      }
    }
    result->as.vec[0].as.integer = vec_used - 1;
    result->as.vec = GC_realloc(result->as.vec, sizeof(*result->as.vec) * vec_used);
    return 0;
  case VECLISP_PAIR:
    result->type = VECLISP_PAIR;
    if (seq.as.pair == NULL) {
      result->as.pair = NULL;
      return 0;
    }
    result->as.pair = NULL;
    r = result;
    FORPAIR(s, &seq) {
      fun_args.type = VECLISP_PAIR;
      fun_args.as.pair = veclisp_alloc_pair();
      fun_args.as.pair[0] = s->as.pair[0];
      fun_args.as.pair[1].type = VECLISP_PAIR;
      fun_args.as.pair[1].as.pair = NULL;
      if (veclisp_lambda(scope, fun, fun_args, &t)) return 1;
      if (t.type != VECLISP_PAIR || t.as.pair != NULL) {
        r->as.pair = veclisp_alloc_pair();
        r->as.pair[0] = s->as.pair[0];
        r->as.pair[1].type = VECLISP_PAIR;
        r->as.pair[1].as.pair = NULL;
        r = &r->as.pair[1];
      }
      if (s->as.pair[1].type != VECLISP_PAIR) {
        fun_args.as.pair = veclisp_alloc_pair();
        fun_args.as.pair[0] = s->as.pair[1];
        fun_args.as.pair[1].type = VECLISP_PAIR;
        fun_args.as.pair[1].as.pair = NULL;
        if (veclisp_lambda(scope, fun, fun_args, &t)) return 1;
        if (t.type != VECLISP_PAIR || t.as.pair != NULL) {
          *r = s->as.pair[1];
        }
      }
    }
    return 0;
  default:
  case VECLISP_INT:
  case VECLISP_SYM:
    result->type = VECLISP_SYM;
    result->as.sym = VECLISP_ERR_INVALID_SEQUENCE;
    return 1;
  }
}
int veclisp_n_let(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a;
  struct veclisp_scope let_scope;
  struct veclisp_bindings let_bindings, *b;
  b = let_scope.bindings = &let_bindings;
  let_bindings.sym = NULL;
  let_bindings.next = NULL;
  FORPAIR(a, &args.as.pair[0]) {
    if (b->sym != NULL) {
      b->next = GC_malloc(sizeof(*b->next));
      b = b->next;
    }
    b->sym = a->as.pair[0].as.sym;
    if (veclisp_eval(scope, a->as.pair[1].as.pair[0], &b->value)) return 1;
    b->next = NULL;
    a = &a->as.pair[1];
  }
  let_scope.next = scope;
  FORPAIR(a, &args.as.pair[1]) if (veclisp_eval(&let_scope, a->as.pair[0], result)) return 1;
  return 0;
}
int veclisp_n_read(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_read(scope, result)) {
    if (result->type == VECLISP_INT && result->as.integer == EOF) return 0;
    return 1;
  }
  return 0;
}
int veclisp_n_throw(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  veclisp_eval(scope, args.as.pair[0], result);
  return 1;
}
int veclisp_n_catch(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a;
  struct veclisp_scope catch_scope;
  struct veclisp_bindings catch_bindings;
  catch_bindings.sym = args.as.pair[0].as.pair[0].as.sym;
  catch_bindings.value.type = VECLISP_PAIR;
  catch_bindings.value.as.pair = NULL;
  catch_bindings.next = NULL;
  catch_scope.bindings = &catch_bindings;
  catch_scope.next = scope;
  FORPAIR(a, &args.as.pair[1]) {
    if (veclisp_eval(scope, a->as.pair[0], result)) {
      catch_bindings.value = *result;
      FORPAIR(a, &args.as.pair[0].as.pair[1]) {
        if (veclisp_eval(&catch_scope, a->as.pair[0], result)) return 1;
      }
      return 0;
    }
  }
  return 0;
}
void veclisp_writebytes(FILE *out, struct veclisp_cell value) {
  int64_t i;
  struct veclisp_cell *v;
  switch (value.type) {
  case VECLISP_INT:
    fputc((char)value.as.integer, out);
    break;
  case VECLISP_SYM:
    fputs(value.as.sym, out);
    break;
  case VECLISP_PAIR:
    FORPAIR(v, &value) {
      veclisp_writebytes(out, v->as.pair[0]);
      if (v->as.pair[1].type != VECLISP_PAIR) veclisp_writebytes(out, v->as.pair[1]);
    }
    break;
  case VECLISP_VEC:
    FORVEC(i, value.as.vec) veclisp_writebytes(out, value.as.vec[i]);
    break;
  }
}
int veclisp_n_writebytes(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, out;
  if (veclisp_scope_lookup(scope, VECLISP_OUTPORT, &out)) {
    out.type = VECLISP_INT;
    out.as.integer = (int64_t)stdout;
  }
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], result)) return 1;
    veclisp_writebytes((FILE *)out.as.integer, *result);
  }
  return 0;
}
void veclisp_print(FILE *out, struct veclisp_cell value) {
  int64_t i;
  struct veclisp_cell *v;
  switch (value.type) {
  case VECLISP_INT:
    fprintf(out, "%li", value.as.integer);
    break;
  case VECLISP_SYM:
    fputs(value.as.sym, out);
    break;
  case VECLISP_VEC:
    FORVEC(i, value.as.vec) veclisp_print(out, value.as.vec[i]);
    break;
  case VECLISP_PAIR:
    FORPAIR(v, &value) {
      veclisp_print(out, v->as.pair[0]);
      if (v->as.pair[1].type != VECLISP_PAIR) veclisp_print(out, v->as.pair[1]);
    }
    break;
  }
}
int veclisp_n_print(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell *a, out;
  if (veclisp_scope_lookup(scope, VECLISP_OUTPORT, &out)) {
    out.type = VECLISP_INT;
    out.as.integer = (int64_t)stdout;
  }
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], result)) return 1;
    veclisp_print((FILE *)out.as.integer, *result);
  }
  return 0;
}
int veclisp_n_exit(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) {
    veclisp_print_err(scope, *result);
    exit(-1);
  }
  if (result->type != VECLISP_INT) {
    args.type = VECLISP_SYM;
    args.as.sym = "exit should be called with an integer";
    veclisp_print_err(scope, args);
    exit(-2);
  }
  exit(result->as.integer);
  return 1;
}
int veclisp_n_write(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  if (veclisp_eval(scope, args.as.pair[0], result)) return 1;
  veclisp_write(scope, *result);
  return 0;
}
char *veclisp_pack(struct veclisp_cell value, int64_t *used, int64_t *allocated, char *sym) {
  int64_t i;
  switch (value.type) {
  case VECLISP_SYM:
    for (i = 0; value.as.sym[i] != 0; ++i) {
      if (*used >= *allocated) {
        *allocated *= 2;
        sym = GC_realloc(sym, sizeof(*sym) * (*allocated));
      }
      sym[(*used)++] = value.as.sym[i];
    }
    return sym;
  case VECLISP_INT:
    if (*used >= *allocated) {
      *allocated *= 2;
      sym = GC_realloc(sym, sizeof(*sym) * (*allocated));
    }
    sym[(*used)++] = value.as.integer;
    return sym;
  case VECLISP_PAIR:
    sym = veclisp_pack(value.as.pair[0], used, allocated, sym);
    return veclisp_pack(value.as.pair[1], used, allocated, sym);
  case VECLISP_VEC:
    FORVEC(i, value.as.vec) {
      sym = veclisp_pack(value.as.vec[i], used, allocated, sym);
    }
    return sym;
  }
  return NULL;
}
int veclisp_n_pack(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell value, *a;
  int64_t used = 0, allocated = 32;
  char *sym = GC_malloc(sizeof(*sym) * allocated);
  FORPAIR(a, &args) {
    if (veclisp_eval(scope, a->as.pair[0], &value)) return 1;
    sym = veclisp_pack(value, &used, &allocated, sym);
  }
  sym[used++] = 0;
  result->type = VECLISP_SYM;
  result->as.sym = veclisp_intern(sym);
  return 0;
}
int veclisp_n_fold(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *nil) {
  int64_t i;
  struct veclisp_cell cons, seq, cons_args, *s;
  if (veclisp_eval(scope, args.as.pair[0], &cons)) {
    *nil = cons;
    return 1;
  }
  if (veclisp_eval(scope, args.as.pair[1].as.pair[0], nil)) return 1;
  if (veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[0], &seq)) {
    *nil = seq;
    return 1;
  }
  switch (seq.type) {
  case VECLISP_INT:
  case VECLISP_SYM:
    nil->type = VECLISP_SYM;
    nil->as.sym = VECLISP_ERR_INVALID_SEQUENCE;
    return 1;
  case VECLISP_PAIR:
    FORPAIR(s, &seq) {
      cons_args.type = VECLISP_PAIR;
      cons_args.as.pair = veclisp_alloc_pair();
      cons_args.as.pair[0] = s->as.pair[0];
      cons_args.as.pair[1].type = VECLISP_PAIR;
      cons_args.as.pair[1].as.pair = veclisp_alloc_pair();
      cons_args.as.pair[1].as.pair[0] = *nil;
      cons_args.as.pair[1].as.pair[1].type = VECLISP_PAIR;
      cons_args.as.pair[1].as.pair[1].as.pair = NULL;
      if (veclisp_lambda(scope, cons, cons_args, nil)) return 1;
    }
    return 0;
  case VECLISP_VEC:
    FORVEC(i, seq.as.vec) {
      cons_args.type = VECLISP_PAIR;
      cons_args.as.pair = veclisp_alloc_pair();
      cons_args.as.pair[0] = seq.as.vec[i];
      cons_args.as.pair[1].type = VECLISP_PAIR;
      cons_args.as.pair[1].as.pair = veclisp_alloc_pair();
      cons_args.as.pair[1].as.pair[0] = *nil;
      cons_args.as.pair[1].as.pair[1].type = VECLISP_PAIR;
      cons_args.as.pair[1].as.pair[1].as.pair = NULL;
      if (veclisp_lambda(scope, cons, cons_args, nil)) return 1;
    }
    return 0;
  }
  return 1;
}
int veclisp_n_no(struct veclisp_scope *scope,  struct veclisp_cell args, struct veclisp_cell *result) {
  result->type = VECLISP_PAIR;
  result->as.pair = NULL;
  return 0;
}
int veclisp_n_yes(struct veclisp_scope *scope,  struct veclisp_cell args, struct veclisp_cell *result) {
  result->type = VECLISP_SYM;
  result->as.sym = VECLISP_T;
  return 0;
}
int veclisp_n_unfoldpair(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  struct veclisp_cell p, f, g, seed, tailgen, *r, s;
  if (veclisp_eval(scope, args.as.pair[0], &p)
      || veclisp_eval(scope, args.as.pair[1].as.pair[0], &f)
      || veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[0], &g)
      || veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[1].as.pair[0], &seed)) {
    return 1;
  } else if (args.as.pair[1].as.pair[1].as.pair[1].as.pair[1].as.pair == NULL) {
    tailgen.type = VECLISP_INT;
    tailgen.as.integer = (int64_t)veclisp_n_no;
  } else if (veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[1].as.pair[1].as.pair[0], &tailgen)) {
    return 1;
  }
  r = result;
  for (;;) {
    args.type = VECLISP_PAIR;
    args.as.pair = veclisp_alloc_pair();
    args.as.pair[0] = seed;
    args.as.pair[1].type = VECLISP_PAIR;
    args.as.pair[1].as.pair = NULL;
    if (veclisp_lambda(scope, p, args, &s)) return 1;
    if (!(s.type == VECLISP_PAIR && s.as.pair == NULL)) {
      args.type = VECLISP_PAIR;
      args.as.pair = veclisp_alloc_pair();
      args.as.pair[0] = seed;
      args.as.pair[1].type = VECLISP_PAIR;
      args.as.pair[1].as.pair = NULL;
      return veclisp_lambda(scope, tailgen, args, r);
    }
    r->type = VECLISP_PAIR;
    r->as.pair = veclisp_alloc_pair();
    args.type = VECLISP_PAIR;
    args.as.pair = veclisp_alloc_pair();
    args.as.pair[0] = seed;
    args.as.pair[1].type = VECLISP_PAIR;
    args.as.pair[1].as.pair = NULL;
    if (veclisp_lambda(scope, f, args, &r->as.pair[0])) return 1;
    r = &r->as.pair[1];
    args.type = VECLISP_PAIR;
    args.as.pair = veclisp_alloc_pair();
    args.as.pair[0] = seed;
    args.as.pair[1].type = VECLISP_PAIR;
    args.as.pair[1].as.pair = NULL;
    if (veclisp_lambda(scope, g, args, &seed)) return 1;
  }
  return 1;
}
int veclisp_n_unfoldvec(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  int64_t used = 1, allocated = 32;
  struct veclisp_cell p, f, g, seed, s;
  if (veclisp_eval(scope, args.as.pair[0], &p)
      || veclisp_eval(scope, args.as.pair[1].as.pair[0], &f)
      || veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[0], &g)
      || veclisp_eval(scope, args.as.pair[1].as.pair[1].as.pair[1].as.pair[0], &seed)) {
    return 1;
  }
  result->type = VECLISP_VEC;
  result->as.vec = GC_malloc(sizeof(*result->as.vec) * allocated);
  result->as.vec[0].type = VECLISP_INT;
  result->as.vec[0].as.integer = 0;
  for (;;) {
    if (used >= allocated) {
      allocated *= 2;
      result->as.vec = GC_realloc(result->as.vec, sizeof(*result->as.vec) * allocated);
    }
    args.type = VECLISP_PAIR;
    args.as.pair = veclisp_alloc_pair();
    args.as.pair[0] = seed;
    args.as.pair[1].type = VECLISP_PAIR;
    args.as.pair[1].as.pair = NULL;
    if (veclisp_lambda(scope, p, args, &s)) return 1;
    if (!(s.type == VECLISP_PAIR && s.as.pair == NULL)) {
      result->as.vec[0].as.integer = used - 1;
      result->as.vec = GC_realloc(result->as.vec, sizeof(*result->as.vec) * used);
      return 0;
    }
    args.type = VECLISP_PAIR;
    args.as.pair = veclisp_alloc_pair();
    args.as.pair[0] = seed;
    args.as.pair[1].type = VECLISP_PAIR;
    args.as.pair[1].as.pair = NULL;
    if (veclisp_lambda(scope, f, args, &result->as.vec[used++])) return 1;
    args.type = VECLISP_PAIR;
    args.as.pair = veclisp_alloc_pair();
    args.as.pair[0] = seed;
    args.as.pair[1].type = VECLISP_PAIR;
    args.as.pair[1].as.pair = NULL;
    if (veclisp_lambda(scope, g, args, &seed)) return 1;
  }
  return 1;
}  
/*
  int veclisp_n_find(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  }
  int veclisp_n_bytes(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  }
  int veclisp_n_readbytes(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  }
  int veclisp_n_readline(struct veclisp_scope *scope, struct veclisp_cell args, struct veclisp_cell *result) {
  }
*/
