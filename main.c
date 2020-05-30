#include <stdio.h>
#include <stdlib.h>

// input buffer
static char input[2048];

int main(int argc, char **argv) {
  // print version information
  puts("mylisp 0.1");
  puts("Press Ctrl+C to exit");

  // REPL
  while (1) {
    fputs(" > ", stdout);
    // read input line
    fgets(input, 2048, stdin);
    // echo
    printf("%s", input);
  }
}
