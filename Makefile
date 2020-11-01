CFLAGS = -I. -Wall -MMD -g3
LDFLAGS = -lpthread -lrt

TARGETS = ProccessSyncBy2Sems ProccessSyncBy1Sem ProccessSyncBy1Mutex ProccessSyncBy1Spin \
ProccessSyncBySignal

all: $(TARGETS)

ProccessSyncBy2Sems: ProccessSyncBy2Sems.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessSyncBy1Sem: ProccessSyncBy1Sem.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessSyncBy1Mutex: ProccessSyncBy1Mutex.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessSyncBy1Spin: ProccessSyncBy1Spin.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ProccessSyncBySignal: ProccessSyncBySignal.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) -c $<

clean:
	rm -rf *.o *~ $(TARGETS) *.d


-include $(wildcard *.d)
