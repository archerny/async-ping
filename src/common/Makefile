ROOT=../..

INCLUDE = \
	-I. \
	-I$(ROOT)/depend/json \
	-I$(ROOT)/src/ \

LIBS = \
	-L. \
	-L$(ROOT)/lib/ \

CFLAG = -c -g -O2 -Wall -fPIC $(INCLUDE) -DSHMEM

GCC = g++
CC = gcc
.SUFFIXES: .o .cpp .c

OBJS = \
	JsonValue.o \
	ThreadWorker.o \
	Common.o \

.PHONY: all clean

all: libcommon.a

libcommon.a: $(OBJS)
	ar -crv ../../lib/$@ $(OBJS)

.cpp.o:
	$(GCC) $(CFLAG) -DLINUX -o $@ $<

clean:
	rm -f *.o
	rm -f ../../lib/libcommon.a
