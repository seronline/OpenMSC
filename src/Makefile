RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lpthread -lboost_thread -lboost_system -L /lib64 -l pthread -lm -lrt -lconfig++ -lboost_regex -llog4cxx -lapr-1 -laprutil-1

SRCS=openmsc.cc receiverDummy.cc readMsc.cc time.cc
OBJS=$(subst .cc,.o,$(SRCS))

openmsc: openmsc.o
	g++ $(LDFLAGS) -o openmsc openmsc.o readMsc.o eventIdGenerator.o dictionary.o time.o $(LDLIBS)
	mv openmsc ../bin
	 
receiverDummy: receiverDummy.o
	g++ $(LDFLAGS) -o receiverDummy receiverDummy.o $(LDLIBS)
	$(RM) receiverDummy.o
	mv receiverDummy ../bin
	
openmsc.o: openmsc.cc
	g++ $(CPPFLAGS) -c time.cc 
	g++ $(CPPFLAGS) -c readMsc.cc 
	g++ $(CPPFLAGS) -c eventIdGenerator.cc
	g++ $(CPPFLAGS) -c dictionary.cc
	g++ $(CPPFLAGS) -c openmsc.cc 

receiverDummy.o: receiverDummy.cc
	g++ $(CPPFLAGS) -c receiverDummy.cc 
	
clean:
	$(RM) $(OBJS) ../bin/openmsc ../bin/receiverDummy
	$(RM) $(OBJS) /usr/bin/openmsc /usr/bin/receiverDummy

install:
	cp ../bin/openmsc /usr/bin
	
all:
	make clean
	make openmsc
	make receiverDummy
