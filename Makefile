CFLAGS = -I. -Wall -MMD -g3
LDFLAGS = -lpthread -lrt

TARGETS = ProccessSyncBy2Sems ProccessRace2 ProccessRace3 ProccessRace4

all: $(TARGETS)

ProccessRace: ProccessRace.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessRace2: ProccessRace2.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessRace3: ProccessRace3.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessRace4: ProccessRace4.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) -c $<

clean:
	rm -rf *.o *~ $(TARGETS) *.d


-include $(wildcard *.d)
