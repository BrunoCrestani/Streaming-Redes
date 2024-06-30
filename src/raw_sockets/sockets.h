#ifndef __SOCKETS__
#define __SOCKETS__

unsigned int rawSocketCreator(char* network_interface_name);
unsigned int rawSocketSend(int rsocket, const void *buffer, unsigned int length, int flags);

#endif
