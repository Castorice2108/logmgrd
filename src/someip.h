#ifndef SOMEIP_H
#define SOMEIP_H

#include <stdint.h>
#include <netinet/in.h>
int someip_listen(const char *ip, uint16_t port);
int someip_serve(int sfd, struct sockaddr_in *from, socklen_t *fromlen, uint32_t *payload);
int someip_reply(const struct sockaddr_in *to, uint32_t sig);

#endif