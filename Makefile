ACSINT = ../acsint
VPATH = ${ACSINT}

CFLAGS += -g
CFLAGS += -I${ACSINT}
LDLIBS = -lespeak -llua

INDENT_SRCS = adapter.c acslua.c adapter.h
SRCS = adapter.c acsbridge.c acslua.c acsbind.c
OBJS = ${SRCS:.c=.o}

all: adapter

adapter: ${OBJS}

clean:
	rm adapter ${OBJS}

indent:
	indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0 ${INDENT_SRCS}
	rm *~
