#Makefile
ROOT=../..

INCLUDE = -I./

#CFLAG = -c -g -pg -Wall -fPIC -DSHMEM $(INCLUDE) -DLINUX
#CFLAG = -c -O3 $(INCLUDE) -DLINUX
CFLAG = -c -g -O3 $(INCLUDE) -DLINUX


GCC = g++
CC = gcc
.SUFFIXES: .o .cpp .c

OBJS = json_reader.o \
	json_writer.o \
	json_value.o

all : $(ROOT)/lib/libjson.a

$(ROOT)/lib/libjson.a : $(OBJS)
	ar -crv ./$@ $(OBJS)
	chmod 644 $(ROOT)/lib/libjson.a

.cpp.o:
	$(GCC) $(CFLAG) -o $@ $<

clean:
	rm -f *.o $(ROOT)/lib/libjson.a 
