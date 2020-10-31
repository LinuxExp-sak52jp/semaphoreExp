CFLAGS = -I. -Wall -MMD -g3
LDFLAGS = -lpthread -lrt

TARGETS = ProccessRace

all: $(TARGETS)

ProccessRace: ProccessRace.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) -c $<

clean:
	rm -rf *.o *~ $(TARGETS) *.d


-include $(wildcard *.d)
