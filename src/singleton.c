#include "singleton.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

/* 单例锁文件实现，返回fd，失败返回-1 */
int singleton_lock(const char *lockfile) {
    int fd = open(lockfile, O_RDWR|O_CREAT, 0644);
    if (fd < 0) {
        fprintf(stderr, "无法打开/创建锁文件: %s (%s)\n", lockfile, strerror(errno));
        return -1;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        char buf[64] = "";
        lseek(fd, 0, SEEK_SET);
        ssize_t n = read(fd, buf, sizeof(buf)-1);
        buf[n>0?n:0] = 0;
        fprintf(stderr, "已有 logmgrd 实例在运行 (PID: %s)\n", buf[0]?buf:"未知");
        close(fd);
        return -1;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%d\n", getpid());
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    write(fd, buf, strlen(buf));
    return fd;
}

void singleton_unlock(int lock_fd, const char *lockfile) {
    if (lock_fd >= 0) {
        close(lock_fd);
    }
    unlink(lockfile);
}