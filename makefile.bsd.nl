CFLAGS = -pedantic-errors -std=c99 -mtune=native -Wall -DlangNL
OBJS = siphash.o utf8.o memory.o util.o base.o collections.o file.o system.o \
       world.o lexer.o parser.o walker.o translator.o citrine.o

.SUFFIXES:	.o .c

all:ctrnl

install: ctrnl
	cp ./ctrnl /usr/bin/ctrnl

ctrnl:	$(OBJS)
	$(CC) $(OBJS) -rdynamic -lm -o ctrnl

clean:
	rm -rf ${OBJS} ctrnl
