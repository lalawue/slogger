/* log_appender_file.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 16:21
 * Last-Updated: 2009/12/03 09:39
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "log_xml.h"
#include "log_shared.h"
#include "log_comm.h"
#include "log_apd_file.h"

#define DLOG(fmt, args...) dlog("apd_file: ", fmt, ##args)
#define DERR(fmt, args...) derr("apd_file: ", fmt, ##args)


/* kbytes to byte, plus 1024 */
#define GET_ROLLING_VAL_OF_BYTES(val) ((val) << 10)


/* description: use pthread mutex locker to make sure thread-safe
 *
 * strategy: protect fp and file size indicator, including open_file and
 *           rolling file functions
 */
static pthread_mutex_t g_apdc_file_pt_mutex = PTHREAD_MUTEX_INITIALIZER;

#define _APDC_FILE_LOCK()                               \
    do {                                                \
        pthread_mutex_lock(&g_apdc_file_pt_mutex);      \
    } while (0)

#define _APDC_FILE_UNLOCK()                             \
    do {                                                \
        pthread_mutex_unlock(&g_apdc_file_pt_mutex);    \
    } while (0)



/* description: construct full path from file name */
static __inline__ void
get_full_path(struct s_file_info *fi, char *buf)
{
    int len;
    time_t now;
    struct tm tms;

    len = sprintf(buf, "%s/", fi->path); /* create file name */

    switch (fi->n_type) {
        case FILE_NAME_TYPE_DATE:
        {
            time(&now);
            localtime_r(&now, &tms);
            strftime(&buf[len], LOGGER_MAX_PATH_LEN - len, (char*)fi->n_val, &tms);
            break;
        }

        case FILE_NAME_TYPE_LOGGER:
        {
            sprintf(&buf[len], fi->n_val, fi->name);
            break;
        }

        case FILE_NAME_TYPE_NAME:
        {
            sprintf(&buf[len], "%s", fi->n_val);
            break;
        }
    }
}


/* description: open file */
static __inline__ int
open_file(struct s_file_info *fi, const char *filename)
{
    if ((fi->fp = fopen(filename, "a")) == NULL) {
        DERR("open %s error: %m\n", filename);
        return LOG_ERR;
    }
    return LOG_OK;
}


/* description: close file */
static __inline__ int
close_file(struct s_file_info *fi)
{
    if (fi && fi->fp) {
        fclose(fi->fp);
        fi->fp = NULL;
        return LOG_OK;
    }
    return LOG_ERR;
}


/* description: get file size */
static __inline__ int
get_file_size(struct s_file_info *fi, const char *filename)
{
    struct stat sb;

    if (stat(filename, &sb) == -1) {
        DERR("stat error: %m\n");
        return 0;
    }

    fi->size = (long) sb.st_size;
    return 1;
}


/* description: rename file */
static __inline__ int
rename_file(struct s_file_info *fi, const char *filename)
{
    char new_name[LOGGER_MAX_PATH_LEN] = {0};

    sprintf(new_name, "%s.old", filename);
            
    if (rename(filename, new_name) != 0) {
        DERR("rename file error: %m\n");
        return LOG_ERR;
    }
    return LOG_OK;
}


/* description: rolling file depends on the rolling type
 *
 * notice     : this function should be protected within the mutex locker
 */
static __inline__ int
rolling_file(struct s_file_info *fi, const char *full_path)
{
    /* check rolling type, and get the file size */
    if (fi->r_type == FILE_ROLLING_TYPE_SIZE) {
        get_file_size(fi, full_path);

        /* ok, now we rename the file, reset relative variables */
        if (fi->size > GET_ROLLING_VAL_OF_BYTES(fi->r_val)) {
            close_file(fi);

            if (rename_file(fi, full_path) == LOG_ERR)
                return LOG_ERR;

            if (open_file(fi, full_path) == LOG_ERR)
                return LOG_ERR;

            get_file_size(fi, full_path);
        }
    }
    else if (fi->r_type == FILE_ROLLING_TYPE_DATE) {
        /* FIXME: do not support now */
        DERR("do not support file rolling type date now !\n");
    }

    return LOG_OK;
}


/* description: open file, get current file size, prepare for rolling
 *
 * FIXME: only support rolling file size now
 */
int
log_file_init(void *uh, struct s_comm *comm)
{
    int ret = LOG_ERR;
    struct s_file_info *fi = (struct s_file_info*)uh;

    if ( !fi ) {
        DERR("param error !\n");
        return LOG_ERR;
    }

    _APDC_FILE_LOCK();
    if (fi->fp == NULL) {
        char *path;

        path = ((struct s_comm*)comm)->buf;
        get_full_path(fi, path);

        if ((ret = open_file(fi, path)) == LOG_ERR)
            goto label_exit;

        if ((ret = rolling_file(fi, path)) == LOG_ERR)
            goto label_exit;
    }
label_exit:
    _APDC_FILE_UNLOCK();
    
    return ret;
}


/* description: write buf content to file, update file size, and rolling
 *              file when needed
 *
 * FIXME: do not implement rolling by date
 */
int
log_file_write(void *uh, struct s_msg_info *mi, struct s_comm *comm)
{
    int bytes, ret = LOG_OK;
    struct s_file_info *fi = (struct s_file_info*)uh;

    _APDC_FILE_LOCK();
    bytes = fwrite(mi->msg_buf, 1, mi->msg_len, fi->fp);
    _APDC_FILE_UNLOCK();

    if (bytes <= 0) {
        DERR("write error !\n");
        return LOG_ERR;
    }
    else {
        fi->size += bytes;      /* no need to lock */
    }

    if ((fi->r_type != FILE_ROLLING_TYPE_NONE) 
        && (fi->size > GET_ROLLING_VAL_OF_BYTES(fi->r_val))) {

        _APDC_FILE_LOCK();
        if (fi->size > GET_ROLLING_VAL_OF_BYTES(fi->r_val)) {
            char *path = mi->msg_buf;
            get_full_path(fi, path);
            ret = rolling_file(fi, path);
        }
        _APDC_FILE_UNLOCK();
    }

    return (ret & (bytes == mi->msg_len));
}


/* description: do nothing */
int
log_file_get_reply(void *uh, struct s_comm *comm)
{
    return 1;
}


/* formater function for deference field type */
static field_format_function
ffm[] = {
    NULL,                       /* FIELD_TYPE_NONE */
    field_format_string,        /* FIELD_TYPE_STRING */
    field_format_date_and_time, /* FIELD_TYPE_DATE */
    field_format_date_and_time, /* FIELD_TYPE_TIME */
    field_format_msg,           /* FIELD_TYPE_MSG */
    NULL,                       /* FIELD_TYPE_INT */
    NULL,                       /* FIELD_TYPE_FLOAT */
};


/* description: generete message format for file */
int
log_file_format(void *uh, struct s_msg_info *mi, va_list va)
{
    struct s_fmt *ft;
    struct s_file_info *fi = (struct s_file_info*)uh;
    
    mi->msg_len = 0;

    for (ft = fi->fmt; ft; ft = ft->next) {
        if ( ffm[ ft->d_type ] )
            ffm[ ft->d_type ](mi, ft, va);
    }

    /* check last newline */
    /* if (mi->msg_buf[mi->msg_len - 1] != '\n' */
    /*     && mi->msg_buf[mi->msg_len - 1] != '\r') { */
        
    /*     mi->msg_buf[mi->msg_len++] = '\n'; */
    /*     mi->msg_buf[mi->msg_len++] = '\0'; */
    /* } */

    return LOG_OK;
}


/* description: file appender fini method */
int
log_file_fini(void *uh, struct s_comm *comm)
{
    int ret;
    struct s_file_info *fi = (struct s_file_info*)uh;

    _APDC_FILE_LOCK();
    ret = close_file(fi);
    _APDC_FILE_UNLOCK();

    return ret;
}


/* log_appender_file.c ends here */
