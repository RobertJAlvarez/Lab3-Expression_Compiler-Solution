CC   = cc
OBJS = build_tree.o backend.o

FILE_NAME ?=

CFLAGS = -O3 -g3 -Wall -Wextra -Werror=format-security -Werror=implicit-function-declaration \
         -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Wconversion

all: main

main: $(OBJS) main.o
	$(CC) -o $@ $^

run: main
	./main

clean:
	rm -f *.o main

build_tree.o: build_tree.c build_tree.h
backend.o: backend.c build_tree.h
main.o: main.c build_tree.h

