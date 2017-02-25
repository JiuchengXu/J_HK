#include "includes.h"

struct bus {
	void (*send)(char *, int);

	char (*recv)(void);
};

static struct bus bus;

void bus_send(char *str, int len)
{
	bus.send(str, len);	
}

char bus_recieve(void)
{
	return bus.recv();
}

void register_bus(void *send, void *recv)
{
	bus.recv = (char (*)(void))recv;
	bus.send = (void (*)(char *, int))send;
}
