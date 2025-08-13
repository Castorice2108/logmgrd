/*
 * logmgrd.c - Log Management Daemon
 * 
 * This is the main daemon that provides log archiving and FTP upload services
 * via SOME/IP protocol. It listens for SOME/IP requests, archives logs,
 * and uploads them to an FTP server.
 */

#define _POSIX_C_SOURCE 200809L  /* Enable POSIX.1-2008 features */

/* Project-specific headers */
#include "config.h"    /* Configuration file parsing */
#include "singleton.h" /* Single instance management */
#include "logger.h"    /* Logging functionality */
#include "archive.h"   /* Log archiving functionality */
#include "ftp.h"       /* FTP upload functionality */
#include "someip.h"    /* SOME/IP protocol handling */

/* Standard C library headers */
#include <stdio.h>     /* Standard I/O functions */
#include <stdlib.h>    /* Standard library functions */
#include <string.h>    /* String manipulation functions */
#include <signal.h>    /* Signal handling */
#include <unistd.h>    /* UNIX standard definitions */
#include <netinet/in.h> /* Internet address family */
#include <sys/types.h> /* System data types */
#include <sys/wait.h>  /* Wait for process termination */
#include <arpa/inet.h> /* Internet operations */
#include <time.h>      /* Time functions */
#include <fcntl.h>     /* File control options */

/* Configuration constants */
#define CONF_FILE "/etc/logmgrd.conf"  /* Default configuration file path */
#define SOMEIP_PORT 30501              /* SOME/IP service port */
#define SOMEIP_IP "0.0.0.0"           /* SOME/IP bind address (all interfaces) */
#define LOCK_FILE "/tmp/logmgrd.pid"   /* PID file for singleton enforcement */

/* Global variables */
logmgrd_conf_t g_conf;                    /* Global configuration structure */
static volatile sig_atomic_t g_stop = 0;  /* Signal-safe stop flag */
static int lock_fd = -1;                  /* File descriptor for PID lock file */

/**
 * Clean up PID file when daemon exits
 * This function is called during normal shutdown or signal handling
 */
void remove_pidfile(void) {
    singleton_unlock(lock_fd, LOCK_FILE);
}

/**
 * Signal handler for graceful shutdown
 * Handles SIGTERM, SIGINT, SIGQUIT, and SIGHUP signals
 * 
 * @param sig: Signal number received
 */
void on_signal(int sig) {
    g_stop = 1;        /* Set atomic flag to stop main loop */
    remove_pidfile();  /* Clean up PID file */
}

/**
 * Daemonize the process - fork and detach from terminal
 * 
 * This function performs the standard UNIX daemon initialization:
 * 1. Fork and exit parent process
 * 2. Start new session (setsid)
 * 3. Ignore SIGHUP signal
 * 4. Fork again and exit parent (prevents zombie)
 * 5. Set umask to 0 for file permissions
 * 6. Change directory to root (/)
 * 7. Close all file descriptors
 * 8. Redirect stdin/stdout/stderr to /dev/null
 */
void daemonize(void) {
    pid_t pid = fork();                    /* First fork */
    if (pid < 0) exit(1);                  /* Fork failed */
    if (pid > 0) exit(0);                  /* Parent exits */
    
    setsid();                              /* Create new session */
    signal(SIGHUP, SIG_IGN);               /* Ignore hangup signal */
    
    pid = fork();                          /* Second fork */
    if (pid < 0) exit(1);                  /* Fork failed */
    if (pid > 0) exit(0);                  /* Parent exits */
    
    umask(0);                              /* Clear file mode mask */
    if (chdir("/") != 0) exit(1);          /* Change to root directory */
    
    /* Close all file descriptors */
    for (int fd = 0; fd < 64; fd++) close(fd);
    
    /* Redirect standard file descriptors to /dev/null */
    open("/dev/null", O_RDWR);             /* stdin  (fd 0) */
    dup(0);                                /* stdout (fd 1) */
    dup(0);                                /* stderr (fd 2) */
}

/**
 * Main function - Entry point of the logmgrd daemon
 * 
 * Process flow:
 * 1. Check for singleton instance (prevent multiple daemons)
 * 2. Load configuration from file
 * 3. Optionally daemonize (unless -f flag is provided)
 * 4. Set up signal handlers for graceful shutdown
 * 5. Start SOME/IP server
 * 6. Main service loop: listen for requests, archive logs, upload via FTP
 * 
 * @param argc: Command line argument count
 * @param argv: Command line arguments (-f for foreground mode)
 * @return: Exit status (0 for success, non-zero for error)
 */
int main(int argc, char **argv) {
    /* Ensure only one instance of the daemon runs */
    lock_fd = singleton_lock(LOCK_FILE);
    if (lock_fd < 0) exit(1); 

    /* Initialize configuration structure */
    memset(&g_conf, 0, sizeof g_conf);
    
    /* Load configuration from file */
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

    /* Daemonize unless -f (foreground) flag is provided */
    if (!(argc > 1 && strcmp(argv[1], "-f") == 0)) {
        daemonize();
    }

    /* Initialize logging system with configuration */
    set_log_conf(&g_conf);
    logmsg("logmgrd started, pid=%d", getpid());

    /* Set up signal handlers for graceful shutdown */
    struct sigaction sa;
    sa.sa_handler = on_signal;             /* Signal handler function */
    sigemptyset(&sa.sa_mask);              /* Clear signal mask */
    sa.sa_flags = 0;                       /* No special flags */
    sigaction(SIGTERM, &sa, NULL);         /* Termination signal */
    sigaction(SIGINT,  &sa, NULL);         /* Interrupt signal (Ctrl+C) */
    sigaction(SIGQUIT, &sa, NULL);         /* Quit signal (Ctrl+\) */
    sigaction(SIGHUP,  &sa, NULL);         /* Hangup signal */

    /* Start SOME/IP server socket */
    int sipfd = someip_listen(SOMEIP_IP, SOMEIP_PORT);
    if (sipfd < 0) {
        logmsg("Failed to open SOME/IP socket: %d", sipfd);
        remove_pidfile();
        exit(2);
    }
    logmsg("Listening for SOME/IP (demo TCP) on %s:%d", SOMEIP_IP, SOMEIP_PORT);

    /* Main service loop - continue until stop signal received */
    while (!g_stop) {
        struct sockaddr_in from;           /* Client address information */
        socklen_t fromlen = sizeof(from);  /* Address structure size */
        uint32_t payload = 0;              /* Request payload from client */
        
        /* Wait for and handle SOME/IP requests */
        int rc = someip_serve(sipfd, &from, &fromlen, &payload);
        if (rc == 0) {  /* Request received successfully */
            char tarfile[512];             /* Buffer for archive filename */
            
            /* Archive logs to a tar file */
            if (archive_logs(&g_conf, tarfile, sizeof tarfile) == 0) {
                /* Upload the archive via FTP */
                if (upload_ftp(&g_conf, tarfile) == 0) {
                    /* Success: Send OK response to client */
                    someip_reply(&from, 0x00000000);
                } else {
                    /* FTP upload failed: Send error response */
                    someip_reply(&from, 0xFFFFFFFF);
                }
                /* Clean up temporary archive file */
                unlink(tarfile);
            } else {
                /* Archive creation failed: Send error response */
                someip_reply(&from, 0xFFFFFFFE);
            }
        }
        /* Sleep briefly to prevent excessive CPU usage */
        usleep(100000);  /* 100ms */
    }
    
    /* Cleanup and shutdown */
    logmsg("logmgrd shutting down");
    remove_pidfile();                      /* Clean up PID file */
    close_logfile();                       /* Close log file */
    close(sipfd);                          /* Close SOME/IP socket */
    return 0;                              /* Exit successfully */
}