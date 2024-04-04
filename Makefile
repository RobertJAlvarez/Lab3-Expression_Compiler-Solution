CC   = cc
OBJS = build_tree.o backend.o

CFLAGS = -O3 -g3 -Wall -Wextra -Werror=format-security -Werror=implicit-function-declaration \
         -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Wconversion

all: compile

compile: $(OBJS) main.o
	$(CC) -o $@ $^

run: compile
	./compile

clean:
	DEL_TXT=$$(find . -name '*.txt' ! -name 'input*' ! -name 'output*'); \
	rm -f *.o compile $$DEL_TXT

build_tree.o: build_tree.c build_tree.h
backend.o: backend.c build_tree.h
main.o: main.c build_tree.h

