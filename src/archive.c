#include "archive.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int archive_logs(const logmgrd_conf_t *conf, char *tarfile, size_t tarsz) {
    time_t t = time(NULL);
    snprintf(tarfile, tarsz, "%s/logs_%ld.tar.gz", conf->tar_path, t);
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "tar czf '%s' -C '%s' . 2>/dev/null", tarfile, conf->log_path);
    logmsg("Archiving logs: %s", cmd);
    int rc = system(cmd);
    if (rc != 0) {
        logmsg("Archive failed: tar returned %d", rc);
        return -1;
    }
    struct stat st;
    if (stat(tarfile, &st) != 0 || st.st_size < 128) {
        logmsg("Archive file not found or too small: %s", tarfile);
        return -2;
    }
    logmsg("Archive created: %s (%ld bytes)", tarfile, st.st_size);
    return 0;
}