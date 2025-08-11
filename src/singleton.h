#ifndef SINGLETON_H
#define SINGLETON_H

int singleton_lock(const char *lockfile);
void singleton_unlock(int lock_fd, const char *lockfile);

#endif