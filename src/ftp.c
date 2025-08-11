#include "ftp.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int upload_ftp(const logmgrd_conf_t *conf, const char *tarfile) {
    char url[512], cmd[2048];
    snprintf(url, sizeof url, "ftp://%s:%d%s", conf->ftp_host, conf->ftp_port, conf->ftp_root);
    size_t len = strlen(url);
    if (url[len-1] == '/' && conf->ftp_root[1] == 0) url[len-1] = 0;
    snprintf(cmd, sizeof cmd,
        "curl -T '%s' --ftp-create-dirs --user '%s:%s' '%s/' --silent --show-error --fail",
        tarfile, conf->ftp_user, conf->ftp_pass, url);
    logmsg("Uploading to FTP: %s", cmd);
    int rc = system(cmd);
    if (rc != 0) {
        logmsg("FTP upload failed: curl returned %d", rc);
        return -1;
    }
    logmsg("FTP upload success: %s", tarfile);
    return 0;
}