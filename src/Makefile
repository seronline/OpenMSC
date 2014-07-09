RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lpthread -lboost_thread -lboost_system -lboost_iostreams -L /lib64 -l pthread -lm -lrt -lconfig++ -lboost_regex -llog4cxx -lapr-1 -laprutil-1

SRCS=openmsc.cc receiverDummy.cc readMsc.cc time.cc
OBJS=$(subst .cc,.o,$(SRCS))

openmsc: openmsc.o
	g++ $(LDFLAGS) -o openmsc openmsc.o readMsc.o eventIdGenerator.o dictionary.o time.o visualiser.o $(LDLIBS)
	mv openmsc ../bin
	 
receiverDummyUdp: receiverDummyUdp.o
	g++ $(LDFLAGS) -o receiverDummyUdp receiverDummyUdp.o $(LDLIBS)
	$(RM) receiverDummyUdp.o
	mv receiverDummyUdp ../bin
	
receiverDummyTcp: receiverDummyTcp.o
	g++ $(LDFLAGS) -o receiverDummyTcp receiverDummyTcp.o $(LDLIBS)
	$(RM) receiverDummyTcp.o
	mv receiverDummyTcp ../bin
	
openmsc.o: openmsc.cc
	g++ $(CPPFLAGS) -c time.cc 
	g++ $(CPPFLAGS) -c readMsc.cc 
	g++ $(CPPFLAGS) -c eventIdGenerator.cc
	g++ $(CPPFLAGS) -c dictionary.cc
	g++ $(CPPFLAGS) -c visualiser.cc
	g++ $(CPPFLAGS) -c openmsc.cc 

receiverDummyUdp.o: receiverDummyUdp.cc
	g++ $(CPPFLAGS) -c receiverDummyUdp.cc 
	
receiverDummyTcp.o: receiverDummyTcp.cc
	g++ $(CPPFLAGS) -c receiverDummyTcp.cc 
	
clean:
	$(RM) $(OBJS) ../bin/openmsc ../bin/receiverDummy

install:
	cp ../bin/openmsc /usr/bin
	
uninstall:
	$(RM) $(OBJS) /usr/bin/openmsc /usr/bin/receiverDummy
	
all:
	make clean
	make openmsc
	make receiverDummyUdp
	make receiverDummyTcp
