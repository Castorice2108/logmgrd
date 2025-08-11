#include "someip.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define UPLOAD_SIGNAL 0xAABBCCDD

int someip_listen(const char *ip, uint16_t port) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    struct sockaddr_in addr;
    if (sfd < 0) return -1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sfd);
        return -2;
    }
    if (listen(sfd, 4) != 0) {
        close(sfd);
        return -3;
    }
    return sfd;
}

int someip_serve(int sfd, struct sockaddr_in *from, socklen_t *fromlen, uint32_t *payload) {
    int cfd = accept(sfd, (struct sockaddr*)from, fromlen);
    if (cfd < 0) return -1;
    uint32_t sig;
    ssize_t r = recv(cfd, &sig, sizeof(sig), MSG_WAITALL);
    if (r != sizeof(sig)) { close(cfd); return 1; }
    sig = ntohl(sig);
    logmsg("Received SOME/IP signal: 0x%08X", sig);
    if (sig == UPLOAD_SIGNAL) {
        *payload = sig;
        close(cfd);
        return 0;
    }
    close(cfd);
    return 1;
}

int someip_reply(const struct sockaddr_in *to, uint32_t sig) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) return -1;
    if (connect(sfd, (struct sockaddr*)to, sizeof(*to)) != 0) {
        close(sfd);
        return -2;
    }
    sig = htonl(sig);
    ssize_t w = send(sfd, &sig, sizeof(sig), 0);
    close(sfd);
    logmsg("Sent reply SOME/IP signal: 0x%08X (ret: %ld)", ntohl(sig), (long)w);
    return (w == sizeof(sig)) ? 0 : -1;
}