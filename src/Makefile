#Makefile
ROOT=..
TARGET=$(ROOT)/bin/xping
CURPATH := ${PWD}/..

INCLUDE=\
	-I. \
	-I$(ROOT)/src/common \
	-I$(ROOT)/depend/json \
	-I$(ROOT)/tmp/libevent-install/include \

LIBS=\
	-L. \
	-L$(ROOT)/lib/ \
	-L$(ROOT)/tmp/libevent-install/lib \

CFLAG = -c -g -O2 -Wall -fPIC $(INCLUDE) -DSHMEM

GCC = g++
CC = gcc
.SUFFIXES: .o .cpp


OBJS = Main.o \
	   HttpServer.o \
	   HttpHandler.o \
	   HttpResponse.o \
	   TaskWorker.o \
	   Log.o \
	   Ping.o \
	   Task.o \

all:  $(TARGET)

$(TARGET): commonlibs $(OBJS)
	$(GCC) -g -o $@ $(OBJS) $(LIBS) -lcommon -levent -ljson -lpthread -lrt

commonlibs: libevent.a
	cd $(ROOT)/depend/json; make
	cd common; make

libevent.a:
	if [ ! -d $(CURPATH)/tmp/libevent-install ] ; then mkdir -p $(CURPATH)/tmp; tar -xzvf $(ROOT)/depend/libevent-2.0.22-stable.tar.gz -C $(CURPATH)/tmp; cd $(CURPATH)/tmp/libevent-2.0.22-stable; ./configure --prefix=$(CURPATH)/tmp/libevent-install/ --disable-shared --disable-openssl; make; make install; fi

# $@ 扩展成当前规则的目的文件名
# $< 扩展成依靠列表中的第 一个依靠文件
.cpp.o:
	$(GCC) $(CFLAG) -DLINUX -o $@ $<

clean:
	rm -f *.o
	rm -f $(ROOT)/bin/*
	cd common; make clean

realclean:
	rm -f *.o
	rm -rf ../tmp
	rm -rf $(ROOT)/bin/*
	rm -rf $(ROOT)/depend/json/*.o
	rm -rf $(ROOT)/lib/*
