#ifndef __BUS_H_
#define __BUS_H_

char bus_recieve(void);
void bus_send(char *str, int len);

void register_bus(void *, void *);

#endif /* __BUS_H_ */
