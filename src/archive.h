#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "config.h"
int archive_logs(const logmgrd_conf_t *conf, char *tarfile, size_t tarsz);

#endif