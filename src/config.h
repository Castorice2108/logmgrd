#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PATH 512

typedef struct {
    char log_path[MAX_PATH];
    char ftp_host[128];
    char ftp_user[64];
    char ftp_pass[64];
    char ftp_root[MAX_PATH];
    int  ftp_port;
    char tar_path[MAX_PATH];
    char log_file[MAX_PATH];
} logmgrd_conf_t;

int load_config(const char *file, logmgrd_conf_t *conf);

#endif