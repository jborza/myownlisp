CC=gcc
lisp:
	$(CC) -o myownlisp -Wall -ledit -lm *.c
