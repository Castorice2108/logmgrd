#include "config.h"
#include "singleton.h"
#include "logger.h"
#include "archive.h"
#include "ftp.h"
#include "someip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <time.h>

#define CONF_FILE "/etc/logmgrd.conf"
#define SOMEIP_PORT 30501
#define SOMEIP_IP "0.0.0.0"
#define LOCK_FILE "/var/run/logmgrd.pid"

logmgrd_conf_t g_conf;
static volatile sig_atomic_t g_stop = 0;
static int lock_fd = -1;

void remove_pidfile(void) {
    singleton_unlock(lock_fd, LOCK_FILE);
}

void on_signal(int sig) {
    g_stop = 1;
    remove_pidfile();
}

void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);
    setsid();
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);
    umask(0);
    chdir("/");
    for (int fd = 0; fd < 64; fd++) close(fd);
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
}

int main(int argc, char **argv) {
    lock_fd = singleton_lock(LOCK_FILE);
    if (lock_fd < 0) exit(1);

    memset(&g_conf, 0, sizeof g_conf);
    int conf_ret = load_config(CONF_FILE, &g_conf);
    if (conf_ret == -1) {
        fprintf(stderr, "配置文件不存在或无法打开: %s\n", CONF_FILE);
        remove_pidfile();
        exit(1);
    }
    if (conf_ret == -2) {
        fprintf(stderr, "配置文件缺少必要项: %s\n", CONF_FILE);
        remove_pidfile();
        exit(1);
    }

    if (!(argc > 1 && strcmp(argv[1], "-f") == 0)) {
        daemonize();
    }

    logmsg("logmgrd started, pid=%d", getpid());

    struct sigaction sa;
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    int sipfd = someip_listen(SOMEIP_IP, SOMEIP_PORT);
    if (sipfd < 0) {
        logmsg("Failed to open SOME/IP socket: %d", sipfd);
        remove_pidfile();
        exit(2);
    }
    logmsg("Listening for SOME/IP (demo TCP) on %s:%d", SOMEIP_IP, SOMEIP_PORT);

    while (!g_stop) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        uint32_t payload = 0;
        int rc = someip_serve(sipfd, &from, &fromlen, &payload);
        if (rc == 0) {
            char tarfile[512];
            if (archive_logs(&g_conf, tarfile, sizeof tarfile) == 0) {
                if (upload_ftp(&g_conf, tarfile) == 0) {
                    someip_reply(&from, 0x00000000);
                } else {
                    someip_reply(&from, 0xFFFFFFFF);
                }
                unlink(tarfile);
            } else {
                someip_reply(&from, 0xFFFFFFFE);
            }
        }
        usleep(100000);
    }
    logmsg("logmgrd shutting down");
    remove_pidfile();
    close(sipfd);
    return 0;
}