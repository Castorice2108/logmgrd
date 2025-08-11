#include "logger.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

static FILE *g_log_fp = NULL;
static pthread_mutex_t g_log_mtx = PTHREAD_MUTEX_INITIALIZER;
logmgrd_conf_t *g_log_conf = NULL; // 需在主程序中赋值

void set_log_conf(logmgrd_conf_t *conf) {
    g_log_conf = conf;
}

void logmsg(const char *fmt, ...) {
    va_list ap;
    time_t now;
    char tbuf[32];

    pthread_mutex_lock(&g_log_mtx);
    if (!g_log_fp) {
        const char *log_file = (g_log_conf && g_log_conf->log_file[0])
            ? g_log_conf->log_file : "/tmp/logmgrd.log";
        g_log_fp = fopen(log_file, "a+");
        if (!g_log_fp) g_log_fp = stderr;
    }
    time(&now);
    strftime(tbuf, sizeof tbuf, "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(g_log_fp, "[%s] ", tbuf);
    va_start(ap, fmt);
    vfprintf(g_log_fp, fmt, ap);
    va_end(ap);
    fprintf(g_log_fp, "\n");
    fflush(g_log_fp);
    pthread_mutex_unlock(&g_log_mtx);
}

void close_logfile(void) {
    pthread_mutex_lock(&g_log_mtx);
    if (g_log_fp && g_log_fp != stderr) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    pthread_mutex_unlock(&g_log_mtx);
}