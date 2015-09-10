
all:ctr

ctr:tools.o lexer.o parser.o walker.o citrine.o
	gcc tools.o lexer.o parser.o walker.o citrine.o -lm -o ctr

tools.o:
	gcc -lbsd -o tools.o -c tools.c -Wall `pkg-config --libs libbsd`

lexer.o:
	gcc -c lexer.c -Wall -o lexer.o

parser.o:
	gcc -c parser.c -Wall -o parser.o

walker.o:
	gcc -c walker.c -Wall -o walker.o

citrine.o:
	gcc -c citrine.c -Wall -o citrine.o

clean:
	rm -rf *.o ctr
	
