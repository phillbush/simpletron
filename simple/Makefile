PROG = simple
SRCS = simple.c token.c expr.c instruction.c symbol.c util.c
OBJS = ${SRCS:.c=.o}

INCS = -I../include

CC = cc
CFLAGS = -Wall ${INCS}
LDFLAGS =

all: ${PROG}

${OBJS}: compiler.h
simple.o: instruction.h symbol.h token.h util.h
instruction.o: instruction.h symbol.h token.h expr.h util.h
expr.o: expr.h token.h util.h
token.o: token.h util.h
util.o: util.h symbol.h
symbol.o: symbol.h util.h

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	-rm *.o ${PROG}

.PHONY: all clean
