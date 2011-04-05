ACSINT = ../acsint
VPATH = ${ACSINT}

CFLAGS += -g
CFLAGS += -I${ACSINT}
LDLIBS = -lespeak -llua

SRCS = adapter.c acsbridge.c acslua.c
OBJS = ${SRCS:.c=.o}

all: adapter

adapter: ${OBJS}

clean:
	rm adapter ${OBJS}
