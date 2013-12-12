RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lpthread -lboost_thread -lboost_system -L /lib64 -l pthread -lm -lrt

SRCS=openmsc.cc
OBJS=$(subst .cc,.o,$(SRCS))

rsm: openmsc.o
	g++ $(LDFLAGS) -o openmsc openmsc.o $(LDLIBS)
	
openmsc.o: openmsc.cc
	g++ $(CPPFLAGS) -c openmsc.cc 
	
clean:
	$(RM) $(OBJS) openmsc

install:
	@echo RTFM! Just run the binary from here douce: ./openmsc -h
	
all:
	make clean
	make
