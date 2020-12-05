/****************************************
 * veclisp.c                            *
 * a lisp with vectors, pairs, ints,    *
 * and symbols                          *
 *                                      *
 * author: jordan@yonder.computer       *
 * license: MIT                         *
 ****************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

char *VECLISP_OUTPORT, *VECLISP_INPORT, *VECLISP_ERRPORT, *VECLISP_PROMPT, *VECLISP_DEFAULT_PROMPT, *VECLISP_QUOTE, *VECLISP_RESPONSE, *VECLISP_DEFAULT_RESPONSE, *VECLISP_ERR_ILLEGAL_DOTTED_LIST, *VECLISP_ERR_EXPECTED_CLOSE_PAREN, *VECLISP_ERR_CANNOT_EXEC_VEC;
char *veclisp_intern(char *sym);
void veclisp_print_prompt(struct veclisp_scope *scope);
void veclisp_write_result(struct veclisp_scope *scope, struct veclisp_cell value);
void veclisp_print_err(struct veclisp_scope *scope, struct veclisp_cell err);
int veclisp_init_root_scope(struct veclisp_scope *root_scope);
int veclisp_scope_lookup(struct veclisp_scope *scope, char *sym, struct veclisp_cell *result);
int veclisp_read(struct veclisp_scope *scope, struct veclisp_cell *result);
int veclisp_eval(struct veclisp_scope *scope, struct veclisp_cell value, struct veclisp_cell *result);
void veclisp_write(struct veclisp_scope *scope, struct veclisp_cell value);
void veclisp_set(struct veclisp_scope *scope, char *interned_sym, struct veclisp_cell value);
void veclisp_fwrite(FILE *out, struct veclisp_cell value);
#define FOREACH(var, init) for (var = init; var != NULL; var = var->next)

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
  FOREACH(s, veclisp_interned_syms) {
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
  VECLISP_INPORT = veclisp_intern("*In");
  VECLISP_OUTPORT = veclisp_intern("*Out");
  VECLISP_ERRPORT = veclisp_intern("*Err");
  VECLISP_PROMPT = veclisp_intern("*Prompt");
  VECLISP_DEFAULT_PROMPT = veclisp_intern("> ");
  VECLISP_QUOTE = veclisp_intern("quote");
  VECLISP_RESPONSE = veclisp_intern("*Response");
  VECLISP_DEFAULT_RESPONSE = veclisp_intern("; ");
  VECLISP_ERR_ILLEGAL_DOTTED_LIST = veclisp_intern("illegal dotted list");
  VECLISP_ERR_EXPECTED_CLOSE_PAREN = veclisp_intern("expected closing parentheses");
  VECLISP_ERR_CANNOT_EXEC_VEC = veclisp_intern("cannot execute a vector. expected integer or pair");
  root_scope->bindings = NULL;
  value.type = VECLISP_INT;
  value.as.integer = (int64_t)stdout;
  veclisp_set(root_scope, VECLISP_OUTPORT, value);
  value.as.integer = (int64_t)stdin;
  veclisp_set(root_scope, VECLISP_INPORT, value);
  value.as.integer = (int64_t)stderr;
  veclisp_set(root_scope, VECLISP_ERRPORT, value);
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
  return malloc(sizeof(struct veclisp_cell) * 2);
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
    result->as.vec = malloc(sizeof(*result->as.vec) * buf_allocated);
    result->as.vec[0].type = VECLISP_INT;
    do {
      if (buf_used >= buf_allocated) {
        buf_allocated *= 2;
        result->as.vec = realloc(result->as.vec, sizeof(*result->as.vec) * buf_allocated);
      }
      c = skip_space((FILE *)inport.as.integer);
      if (c == ']') {
        result->as.vec = realloc(result->as.vec, sizeof(*result->as.vec) * buf_used);
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
      if (isspace(c)) break;
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
  struct veclisp_cell head;
  switch (value.type) {
  case VECLISP_INT:
    *result = value;
    return 0;
  case VECLISP_SYM:
    veclisp_scope_lookup(scope, value.as.sym, result);
    return 0;
  case VECLISP_VEC:
    result->type = VECLISP_VEC;
    result->as.vec = malloc(sizeof(struct veclisp_cell) * (value.as.vec[0].as.integer + 1));
    result->as.vec[0] = value.as.vec[0];
    for (i = 1; i <= value.as.vec[0].as.integer; ++i)
      if (veclisp_eval(scope, value.as.vec[i], &result->as.vec[i])) return 1;
    return 0;
  case VECLISP_PAIR:
    if (value.as.pair == NULL) {
      *result = value;
      return 0;
    }
    head = value.as.pair[0];
  retry:
    if (veclisp_eval(scope, head, &head)) return 1;
    switch (head.type) {
    case VECLISP_VEC:
      result->type = VECLISP_SYM;
      result->as.sym = VECLISP_ERR_CANNOT_EXEC_VEC;
      return 1;
    case VECLISP_SYM: goto retry;
    case VECLISP_INT:
      return ((veclisp_native_func)head.as.integer)(scope, value.as.pair[1], result);
    case VECLISP_PAIR:
      // lambda call expects one of the following forms:
      // ((A B C) body ...)       <- binds the values of its arguments to multiple names
      // (Arg macro-body ...)     <- binds the tail of argument list to a single name
      // ([A B C] macro-body ...) <- binds the unevaluated arguments to names
      // TODO
      return 1;
    }
    return 0;
  default:
    return 1;
  }
}
int veclisp_scope_lookup(struct veclisp_scope *scope, char *sym, struct veclisp_cell *result) {
  struct veclisp_scope *s;
  struct veclisp_bindings *b;
  FOREACH(s, scope) {
    FOREACH(b, s->bindings) {
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
  struct veclisp_scope *s;
  struct veclisp_bindings *b;
  FOREACH(s, scope) {
    if (s->bindings) {
      FOREACH(b, s->bindings) {
        if (b->sym == interned_sym) {
          b->value = value;
          return;
        }
      }
    } else if (!s->next) {
      s->bindings = malloc(sizeof(*scope->bindings));
      s->bindings->sym = interned_sym;
      s->bindings->value = value;
      s->bindings->next = NULL;
      return;
    }
  }
  b = malloc(sizeof(*b));
  b->sym = interned_sym;
  b->value = value;
  b->next = scope->bindings;
  scope->bindings = b;
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
