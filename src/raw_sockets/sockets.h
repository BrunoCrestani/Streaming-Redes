#ifndef __SOCKETS__
#define __SOCKETS__

int rawSocketCreator(char *network_interface_name);
int rawSocketSend(int rsocket, const void *buffer, unsigned int length);

#endif
