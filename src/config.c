#include "config.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_config(const char *file, logmgrd_conf_t *conf) {
    FILE *fp = fopen(file, "r");
    char line[1024];
    int found_log_path = 0, found_ftp_host = 0, found_ftp_user = 0, found_ftp_pass = 0;
    if (!fp) return -1;

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (str_starts(line, "LOG_PATH="))      { strncpy(conf->log_path, line+9, MAX_PATH-1); found_log_path=1; }
        else if (str_starts(line, "FTP_HOST=")) { strncpy(conf->ftp_host, line+9, sizeof(conf->ftp_host)-1); found_ftp_host=1; }
        else if (str_starts(line, "FTP_USER=")) { strncpy(conf->ftp_user, line+9, sizeof(conf->ftp_user)-1); found_ftp_user=1; }
        else if (str_starts(line, "FTP_PASS=")) { strncpy(conf->ftp_pass, line+9, sizeof(conf->ftp_pass)-1); found_ftp_pass=1; }
        else if (str_starts(line, "FTP_PORT=")) conf->ftp_port = atoi(line+9);
        else if (str_starts(line, "FTP_ROOT=")) strncpy(conf->ftp_root, line+9, MAX_PATH-1);
        else if (str_starts(line, "TAR_PATH=")) strncpy(conf->tar_path, line+9, MAX_PATH-1);
        else if (str_starts(line, "LOG_FILE=")) strncpy(conf->log_file, line+9, MAX_PATH-1);
    }
    fclose(fp);
    if (!found_log_path || !found_ftp_host || !found_ftp_user || !found_ftp_pass) return -2;
    if (!conf->tar_path[0]) strcpy(conf->tar_path, "/tmp");
    if (!conf->ftp_root[0]) strcpy(conf->ftp_root, "/");
    if (!conf->log_file[0]) strcpy(conf->log_file, "/tmp/logmgrd.log");
    if (conf->ftp_port == 0) conf->ftp_port = 21;
    return 0;
}