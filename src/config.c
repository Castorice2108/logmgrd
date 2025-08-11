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

    memset(conf, 0, sizeof(*conf)); // 防止未初始化

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (str_starts(line, "LOG_PATH="))      { snprintf(conf->log_path, sizeof(conf->log_path), "%s", line+9); found_log_path=1; }
        else if (str_starts(line, "FTP_HOST=")) { snprintf(conf->ftp_host, sizeof(conf->ftp_host), "%s", line+9); found_ftp_host=1; }
        else if (str_starts(line, "FTP_USER=")) { snprintf(conf->ftp_user, sizeof(conf->ftp_user), "%s", line+9); found_ftp_user=1; }
        else if (str_starts(line, "FTP_PASS=")) { snprintf(conf->ftp_pass, sizeof(conf->ftp_pass), "%s", line+9); found_ftp_pass=1; }
        else if (str_starts(line, "FTP_PORT=")) conf->ftp_port = atoi(line+9);
        else if (str_starts(line, "FTP_ROOT=")) snprintf(conf->ftp_root, sizeof(conf->ftp_root), "%s", line+9);
        else if (str_starts(line, "TAR_PATH=")) snprintf(conf->tar_path, sizeof(conf->tar_path), "%s", line+9);
        else if (str_starts(line, "LOG_FILE=")) snprintf(conf->log_file, sizeof(conf->log_file), "%s", line+9);
    }
    fclose(fp);
    if (!found_log_path || !found_ftp_host || !found_ftp_user || !found_ftp_pass) return -2;
    if (!conf->tar_path[0]) snprintf(conf->tar_path, sizeof(conf->tar_path), "/tmp");
    if (!conf->ftp_root[0]) snprintf(conf->ftp_root, sizeof(conf->ftp_root), "/");
    if (!conf->log_file[0]) snprintf(conf->log_file, sizeof(conf->log_file), "/tmp/logmgrd.log");
    if (conf->ftp_port == 0) conf->ftp_port = 21;
    return 0;
}