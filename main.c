#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char *readline(char *prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char *output = malloc(strlen(buffer) + 1);
  strcpy(outout, buffer);
  // terminate the output string
  output[strlen(output)+1) = '\0';
  return output;
}

// fake add_history function
void add_history(char *history) {}
#else
// include editline headers for editline
#include <editline/readline.h>
#ifdef __linux__
#include <editline/history.h>
#endif
#endif

typedef struct {
  int type;
  long num;
  // error and symbol data
  char *err;
  char *sym;

  // cells
  int cell_count;
  struct lval **cells;
} lval;

// possible lval types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval *lval_err(char *m) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->cell_count = 0;
  v->cells = NULL;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;
  // delete all list elements in case of sexpression
  case LVAL_SEXPR:
    for (int i = 0; i < v->cell_count; i++) {
      lval_del(v->cells[i]);
    }
    free(v->cells);
    break;
  }
}

void lval_print(lval *v);
void lval_expr_print(lval *v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->cell_count; i++) {
    // print the child value
    lval_print(v->cells[i]);

    // no trailing space for the last element
    if (i != (v->cell_count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

// print an lval
void lval_print(lval *v) {
  switch (v->type) {
  // print if it's a number
  case LVAL_NUM:
    printf("%li", v->num);
    break;

  case LVAL_ERR:
      printf("Error: %s", v->err);
      break;

  case LVAL_SYM:
    printf("%s", v->sym);
    break;

  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
  }
}

void lval_println(lval *v) {
  lval_print(v);
  putchar('\n');
}

/*lval eval_op(lval x, char *op, lval y) {
  if (x.type == LVAL_ERR) {
    return x;
  }
  if (y.type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    return lval_num(x.num + y.num);
  }
  if (strcmp(op, "-") == 0) {
    return lval_num(x.num - y.num);
  }
  if (strcmp(op, "*") == 0) {
    return lval_num(x.num * y.num);
  }
  if (strcmp(op, "/") == 0) {
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  return lval_err(LERR_BAD_OP);
}
*/

lval *lval_read_num(mpc_ast_t *t) {
  // check for error in conversion
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval *lval_add(lval *v, lval *child) {
  v->cell_count++;
  v->cells = realloc(v->cells, sizeof(lval *) * v->cell_count);
  v->cells[v->cell_count - 1] = child;
  return v;
}

lval* lval_read(mpc_ast_t *t) {
  // return numbers directly
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }

  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  // if root or sexpr, create empty list
  lval *x = NULL;
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }

  // fill list of subexpressions
  for (int i = 0; i < t->children_num; i++) {
    // skip over parentheses
    if ((strcmp(t->children[i]->contents, "(") == 0) ||
        (strcmp(t->children[i]->contents, ")") == 0)) {
      continue;
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

int main(int argc, char **argv) {
  // create parsers
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispy = mpc_new("lispy");
  // define parsers
  mpca_lang(MPCA_LANG_DEFAULT, "                                         \
          number   : /-?[0-9]+/ ;                     \
          symbol   : '+' | '-' | '*' | '/' ;          \
          sexpr    : '(' <expr>* ')' ;                \
          expr     : <number> | <symbol> | <sexpr> ;  \
          lispy    : /^/ <expr>* /$/ ;     \
          ",
            Number, Symbol, Sexpr, Expr, Lispy);

  // print version information
  puts("mylisp 0.1");
  puts("Press Ctrl+C to exit");

  // REPL
  while (1) {
    char *input = readline(" > ");
    add_history(input);

    // echo
    // printf("%s \n", input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      // print AST on success
      mpc_ast_print(r.output);

      // load AST from output
      //lval result = eval(r.output);
      lval* x = lval_read(r.output);
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    // clean up
    free(input);
  }
  // clean up parsers
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
}
