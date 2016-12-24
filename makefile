# makefile - for the shell project used in
# CS570 Fall 2016
# San Diego State University


PROGRAM = p2
CC = gcc
CFLAGS = -g

${PROGRAM}:	${PROGRAM}.o getword.o
		${CC} ${CFLAGS} ${PROGRAM}.o getword.o -o ${PROGRAM} -lm

getword.o:	getword.h
${PROGRAM}.o:	${PROGRAM}.h getword.h

clean:
		rm -f *.o ${PROGRAM} your.output*

lint:
		lint getword.c ${PROGRAM}.c
