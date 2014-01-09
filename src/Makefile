RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lpthread -lboost_thread -lboost_system -L /lib64 -l pthread -lm -lrt -lconfig++ -lboost_regex

SRCS=openmsc.cc receiverDummy.cc msc.cc 
OBJS=$(subst .cc,.o,$(SRCS))

openmsc: openmsc.o
	g++ $(LDFLAGS) -o openmsc openmsc.o msc.o $(LDLIBS)
	$(RM) openmsc.o msc.o
	
receiverDummy: receiverDummy.o
	g++ $(LDFLAGS) -o receiverDummy receiverDummy.o $(LDLIBS)
	$(RM) receiverDummy.o
	
openmsc.o: openmsc.cc
	g++ $(CPPFLAGS) -c msc.cc 
	g++ $(CPPFLAGS) -c openmsc.cc 

receiverDummy.o: receiverDummy.cc
	g++ $(CPPFLAGS) -c receiverDummy.cc 
	
clean:
	$(RM) $(OBJS) openmsc receiverDummy msc

install:
	@echo Nothing to do! Just run the binary from here: ./openmsc -h
	
all:
	make clean
	make openmsc
	make receiverDummy
