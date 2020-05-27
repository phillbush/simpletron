PROG = simpletron
SRCS = simpletron.c
OBJS = ${SRCS:.c=.o}

INCS = -I../include

CFLAGS = -Wall ${INCS}
LDFLAGS =

all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

simpletron.o: simpletron.h ../include/memory.h

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	-rm ${OBJS} ${PROG}

.PHONY: all clean
