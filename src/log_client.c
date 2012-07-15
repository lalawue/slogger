/* log_clients.c --- 
 * 
 * Copyright (c) 2009 kio_34@kio_34.com. 
 * 
 * Author:  Su Chang (kio_34@kio_34.com)
 * Maintainer: 
 * 
 * Created: 2009/10/30 14:24
 * Last-Updated: 2009/12/02 17:54
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include "log_client.h"
#include "log_xml.h"
#include "log_shared.h"
#include "log_comm.h"
#include "log_apd_std.h"
#include "log_apd_file.h"
#include "log_apd_db.h"

#define DLOG(fmt, args...) dlog("log_cnt: ", fmt, ##args)
#define DERR(fmt, args...) derr("log_cnt: ", fmt, ##args)

/* struct for client log operation */
struct s_logc {
    int connected;              /* connected state */
    int con_fd;                 /* socket fd */
    struct s_logger *lgr;       /* xml conf instance  */
};


/* description: connect server, return the created socket */
static int
connect_server(logc_t *lg)
{
    int con_fd;
    struct sockaddr_un sunx;

    if (lg->connected) {
        DERR("already connect !\n");
        return 0;
    }

    con_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (con_fd == -1) {
        DERR("open socket err !\n");
        goto err_out;
    } 

    sunx.sun_family = AF_UNIX;
    strncpy(sunx.sun_path, LOGGER_PATH, sizeof(sunx.sun_path));

    if (connect(con_fd, (struct sockaddr*)&sunx, SUN_LEN(&sunx)) == -1) {
        DERR("connect socket error: %m\n");
        goto err_out;
    }

    signal(SIGPIPE, SIG_IGN);   /* ignore SIGPIPE */

    /* no need to set non-block */
    /* set_non_blocking(comm->fd); */

    lg->con_fd = con_fd;
    return con_fd;
    
err_out:
    return 0;
}


/* description: disconnect server */
static void
disconnect_server(logc_t *lg)
{
    close(lg->con_fd);
    lg->connected = 0;
}


/* client side appender interfaces */
typedef int (*appender_init)(void *uh, struct s_comm *comm);
typedef int (*appender_format)(void *uh, struct s_msg_info *mi, va_list va);
typedef int (*appender_write)(void *uh, struct s_msg_info *mi, struct s_comm *comm);
typedef int (*appender_get_reply)(void *uh, struct s_comm *comm);
typedef int (*appender_fini)(void *uh, struct s_comm *comm);

/* client side appender structure */
struct s_appender_function {
    appender_init init;
    appender_format format;
    appender_write write;
    appender_get_reply get_reply;
    appender_fini fini;
};


/* appender instances */
static const struct s_appender_function
g_af[] = {
    {NULL},                     /* APD_TYPE_NONE */
    {                           /* APD_TYPE_STDOUT */
        log_std_init,
        log_std_format, 
        log_std_write, 
        log_std_get_reply,
        log_std_fini
    },
    {                           /* APD_TYPE_STDERR */
        log_std_init,
        log_std_format, 
        log_std_write,
        log_std_get_reply,
        log_std_fini
    },
    {                           /* APD_TYPE_FILE */
        log_file_init, 
        log_file_format, 
        log_file_write, 
        log_file_get_reply, 
        log_file_fini
    },
    {                           /* APD_TYPE_DB */
        log_client_db_init,
        log_client_db_format,
        log_client_db_write,
        log_client_db_get_reply,
        log_client_db_fini
    },
    {NULL}
};


/* description: create client logger instance */
logc_t*
log_open(const char *xml)
{
    logc_t *lg;
    char msg_buf[LOGGER_MAX_MSG_LEN];

    struct s_comm comm;
    struct s_appender *apd;

    if (xml == NULL) {
        DERR("no xml provide !\n");
        return NULL;
    }

    /* set debug level at the very beginning */
    get_debug_level();


    /* malloc log client resources */
    lg = (logc_t*)malloc(sizeof(*lg));
    if ( !lg ) {
        DERR("malloc error !\n");
        goto err_malloc;
    }
    memset(lg, 0, sizeof(*lg));


    /* obtain logger option from xml */
    lg->lgr = logc_create_logger(xml);
    if (lg->lgr == NULL) {
        DERR("create logger error !\n");
        goto err_lg;
    }


    /* testing when we enter debug mode*/
    extern int g_debug;
    if (g_debug)
        logc_xml_test_logger(lg->lgr);


    /* init comm struct */
    comm.buf = msg_buf;
    comm.buf_len = sizeof(msg_buf);
    comm.cnt_len = 0;


    /* init appender */
    for (apd = lg->lgr->apd; apd; apd = apd->next) {

        /* connect network when needed */
        if (apd->network && !lg->connected) {
            if ((lg->con_fd = connect_server(lg)) <= 0)
                goto err_lgr;
            lg->connected = 1;
            comm.con_fd = lg->con_fd; /* update comm fd */
        }

        if ( !g_af[ apd->type ].init(apd->info, &comm) ) {
            DERR("log appender init error, apd type %d !\n", apd->type);
            goto err_connect;
        }
    }

    return lg;

err_connect:
    if (lg->connected)
        disconnect_server(lg);
err_lgr:
    logc_destroy_logger(lg->lgr);
err_lg:
    free(lg);
err_malloc:
    return NULL;
}


/* description: destroy logger instance */
void
log_close(logc_t *lg)
{
    struct s_comm comm;
    struct s_appender *apd;

    if ( lg ) {
        char msg_buf[LOGGER_MAX_MSG_LEN];

        /* init comm */
        comm.buf = msg_buf;
        comm.buf_len = sizeof(msg_buf);
        comm.cnt_len = 0;
        comm.con_fd = lg->con_fd;
        
        /* fini appender */
        if (lg->lgr) {
            for (apd = lg->lgr->apd; apd; apd = apd->next) {
                if (g_af[ apd->type ].fini(apd->info, &comm) == LOG_ERR)
                    DERR("appender fini error, apd type %d !\n", apd->type);
            }
        }

        /* disconnect */
        if (lg->connected)
            disconnect_server(lg);

        /* destroy xml logger instance */
        logc_destroy_logger(lg->lgr);

        free(lg);
    }
}


/* description: write log with specific pri and format */
int
log_write(logc_t *lg, int pri, const char *msg_fmt, ...)
{
    time_t now;
    va_list va;
    int ret = LOG_OK;
    char msg_buf[LOGGER_MAX_MSG_LEN];

    struct s_comm comm;
    struct s_msg_info mi;       /* message info */

    struct tm tms;
    struct s_appender *apd;


    if (!(lg && msg_fmt) || (pri < PRI_EMERG) || (pri > PRI_DEBUG)) {
        DERR("parameter error !\n");
        return LOG_ERR;
    }

    time(&now);                 /* generate time stamp */
    localtime_r(&now, &tms);

    /* init comm */
    comm.con_fd = lg->con_fd;
    comm.buf = msg_buf;
    comm.buf_len = sizeof(msg_buf);
    comm.cnt_len = 0;

    /* init message information */
    mi.msg_buf = comm.buf;
    mi.msg_len = 0;
    mi.buf_len = comm.buf_len;
    mi.tm = &tms;
    mi.pri = pri;
    mi.msg_fmt = msg_fmt;
    mi.name = lg->lgr->name;

    va_start(va, msg_fmt);
    for (apd = lg->lgr->apd; apd; apd = apd->next) {

        /* whether to ignore this message */
        if ( !(apd->pri_mask & LOG_GET_MASK(pri)) )
            continue;
        
        /* format and write to appender */
        g_af[ apd->type ].format(apd->info,  &mi, va);
        ret &= g_af[ apd->type ].write(apd->info, &mi, &comm);

        /* get reply */
        ret &= g_af[ apd->type ].get_reply(apd->info, &comm);
    }
    va_end(va);

    return ret;
}


/* log_clients.c ends here */
