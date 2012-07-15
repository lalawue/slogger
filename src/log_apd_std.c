/* log_appender_std.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 18:07
 * Last-Updated: 2009/12/02 17:22
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "log_comm.h"
#include "log_apd_std.h"

/* description: use pthread mutex locker to make sure thread-safe
 *
 * strategy: protect file descriptor
 */
static pthread_mutex_t g_apdc_std_pt_mutex = PTHREAD_MUTEX_INITIALIZER;

#define _APDC_STD_LOCK()                                \
    do {                                                \
        pthread_mutex_lock(&g_apdc_std_pt_mutex);       \
    } while (0)

#define _APDC_STD_UNLOCK()                              \
    do {                                                \
        pthread_mutex_unlock(&g_apdc_std_pt_mutex);     \
    } while (0)


/* description: do nothing, stdout/stderr is always opened */
int 
log_std_init(void *uh, struct s_comm *comm)
{
    return LOG_OK;
}


/* description: write to std, use mutex locker to ensure atomic */
int
log_std_write(void *uh, struct s_msg_info *mi, struct s_comm *comm)
{
    int bytes;
    struct s_std_info *si = (struct s_std_info*)uh;

    _APDC_STD_LOCK();
    bytes = write(si->fd, mi->msg_buf, mi->msg_len);
    _APDC_STD_UNLOCK();
    
    return (bytes == mi->msg_len);
}


/* description: do nothing */
int
log_std_get_reply(void *uh, struct s_comm *comm)
{
    return 1;
}


/* formater function for deference field type */
static field_format_function
sfm[] = {
    NULL,                       /* FIELD_TYPE_NONE */
    field_format_string,        /* FIELD_TYPE_STRING */
    field_format_date_and_time, /* FIELD_TYPE_DATE */
    field_format_date_and_time, /* FIELD_TYPE_TIME */
    field_format_msg,           /* FIELD_TYPE_MSG */
    NULL,                       /* FIELD_TYPE_INT */
    NULL,                       /* FIELD_TYPE_FLOAT */
};


/* description: std format message function */
int
log_std_format(void *uh, struct s_msg_info *mi, va_list va)
{
    struct s_fmt *ft;
    struct s_std_info *si = (struct s_std_info*)uh;

    mi->msg_len = 0;

    for (ft = si->fmt; ft; ft = ft->next) {
        if ( sfm[ ft->d_type ] )
            sfm[ ft->d_type ](mi, ft, va);
    }

    /* check last newline */
    /* if (mi->msg_buf[mi->msg_len - 1] != '\n' */
    /*     && mi->msg_buf[mi->msg_len - 1] != '\r') { */
        
    /*     mi->msg_buf[mi->msg_len++] = '\n'; */
    /*     mi->msg_buf[mi->msg_len++] = '\0'; */
    /* } */
    
    return LOG_OK;
}


/* description: do nothing */
int
log_std_fini(void *uh, struct s_comm *comm)
{
    return LOG_OK;
}


/* log_appender_std.c ends here */
