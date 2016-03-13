# NOTE: Feel free to change the makefile to suit your own need.

# compile and link flags
CCFLAGS = -Wall -g -rdynamic
LDFLAGS = -Wall -g -rdynamic

# make rules
TARGETS = rdt_sim 

all: $(TARGETS)

.cc.o:
	g++ $(CCFLAGS) -c -o $@ $<

checksum.o: 	checksum.h

rdt_sender.o: 	rdt_struct.h rdt_sender.h checksum.h

rdt_receiver.o:	rdt_struct.h rdt_receiver.h checksum.h

rdt_sim.o: 	rdt_struct.h

rdt_sim: rdt_sim.o rdt_sender.o rdt_receiver.o checksum.o
	g++ $(LDFLAGS) -o $@ $^

clean:
	rm -f *~ *.o $(TARGETS)
