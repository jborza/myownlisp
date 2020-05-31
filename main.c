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
  //error and symbol data
  char* err;
  char* sym;

  // cells
  int cell_count;
  struct lval** cells;
} lval;

// possible lval types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

// print an lval
void lval_print(lval v) {
  switch (v.type) {
  // print if it's a number
  case LVAL_NUM:
    printf("%li", v.num);
    break;

  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) {
      printf("Error: divide by zero!");
    }
    if (v.err == LERR_BAD_OP) {
      printf("Error: invalid operator!");
    }

    if (v.err == LERR_BAD_NUM) {
      printf("Error: invalid number!");
    }
    break;
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op(lval x, char *op, lval y) {
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

lval eval(mpc_ast_t *t) {
  // return numbers directly
  if (strstr(t->tag, "number")) {
    // check for error in conversion
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  // operator is a second child of expr
  char *op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  // combine remaining children
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
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
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                         \
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
      mpc_ast_t *a = r.output;
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    // clean up
    free(input);
  }
  // clean up parsers
  mpc_cleanup(5, Number, Symbol, Sexpr , Expr, Lispy);
}
