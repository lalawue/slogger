/* log_appender_db.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 18:29
 * Last-Updated: 2009/12/02 16:58
 * 
 */

/* Commentary: 
 * 
 * client side db appender
 */

/* Code: */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include "log_comm.h"
#include "log_apd_db.h"

#define DLOG(fmt, args...) dlog("apdc_db: ", fmt, ##args)
#define DERR(fmt, args...) derr("apdc_db: ", fmt, ##args)


/* descroption: use pthread mutex locker to make sure thread-safe
 *
 * strategy: protect socket descroptor for reading/writing
 */
static pthread_mutex_t g_apdc_db_pt_mutex = PTHREAD_MUTEX_INITIALIZER;

#define _APDC_DB_LOCK()                                 \
    do {                                                \
        pthread_mutex_lock(&g_apdc_db_pt_mutex);        \
    } while (0)

#define _APDC_DB_UNLOCK()                               \
    do {                                                \
        pthread_mutex_unlock(&g_apdc_db_pt_mutex);      \
    } while (0)


/* description: client apd db comm send, return LOG_OK or LOG_ERR */
int
_apd_db_comm_send(struct s_comm *comm, int mode, int apd_mode, int command, const char *val)
{
    int ret, msg_len = COMM_DB_HEADER_LEN;
    
    if (val)
        msg_len += sprintf(&comm->buf[msg_len], "%s", val);

    log_comm_set_header(comm, mode, msg_len, COMM_DB_APPENDER, apd_mode);

    comm->buf[COMM_HEADER_LEN] = command;

    _APDC_DB_LOCK();
    ret = log_comm_send_raw(comm, msg_len);
    _APDC_DB_UNLOCK();

    return (ret == msg_len);
}


/* description: get reply, return LOG_OK or LOG_ERR */
int
_apd_db_comm_get_reply(struct s_comm *comm)
{
    char *p;
    int ret;

    _APDC_DB_LOCK();
    ret = log_comm_read_raw(comm);
    _APDC_DB_UNLOCK();

    if (ret <= 0)
        return ret;

    p = &comm->buf[COMM_DB_HEADER_LEN];

    /* DLOG("notice result %s, %d\n", p, strcmp(p, COMM_RET_OK)); */

    /* DLOG("mode %x, len %d, module %d, sub_mode %d, command %d\n",  */
    /*      p[0], (p[1]<<8 | p[2]), (p[3] <<8 | p[4]), (p[5]<<8 | p[6]), p[7]); */

    return (!strcmp(p, COMM_RET_OK));
}


/* description: mysql type name, the sequence follow the log_xml.h's
 *              XML_FIELD_TYPE_*
 */
static char
*mysql_type_name[] = {
    NULL                       /* XML_FIELD_TYPE_NONE */
    , "varchar"                /* XML_FIELD_TYPE_STRING */
    , "date"                   /* XML_FIELD_TYPE_DATE */
    , "time"                   /* XML_FIELD_TYPE_TIME */
    , "text"                   /* XML_FIELD_TYPE_TEXT */
    , "int"                    /* XML_FIELD_TYPE_INT */
    , "float"                  /* XML_FIELD_TYPE_FLOAT */
};


/* description: generate create table SQL command when connect server to
 *              init db
 */
static int
generate_and_send_server_side_create_table_command(struct s_db_info *di, struct s_comm *comm)
{
    const struct s_fmt *ft;
    const struct s_field_pair *fr;

    int ret, msg_len = COMM_DB_HEADER_LEN;

    msg_len += sprintf(&comm->buf[msg_len], "CREATE TABLE %s (", di->table);

    for (ft = di->fmt; ft; ft = ft->next) {

        fr = get_field_pair_from_idx(ft->m_idx);
        if (fr == NULL)
            continue;

        /* field name */
        msg_len += sprintf(&comm->buf[msg_len], "%s ", fr->name);

        /* type name (size) */
        msg_len += sprintf(&comm->buf[msg_len], "%s", mysql_type_name[fr->d_type]);

        if (fr->d_type == XML_FIELD_TYPE_STRING || fr->d_type == XML_FIELD_TYPE_INT) {
            msg_len += sprintf(&comm->buf[msg_len], "(%d)", fr->d_size);
        }

        if (ft->next != NULL) {
            msg_len += sprintf(&comm->buf[msg_len], ", ");
        }
    }

    /* at the end */
    msg_len += sprintf(&comm->buf[msg_len], ");");

    /* construct command */
    log_comm_set_header(comm, COMM_CMD_MODE, msg_len, COMM_DB_APPENDER, COMM_APD_CMD);

    comm->buf[COMM_HEADER_LEN] = COMM_DB_CMD_TABLE_FMT;

    _APDC_DB_LOCK();
    ret = log_comm_send_raw(comm, msg_len);;
    _APDC_DB_UNLOCK();

    return ret;
}


/* (apd_mode, command, val) pair, the sequece is what server side will
 * recieved
 */
struct s_db_cmd_seq {
    int apd_mode;
    char command;
    char *val;
};


/* description: init database, send command to server */
int
log_client_db_init(void *uh, struct s_comm *comm)
{
    int i;
    struct s_db_info *di = (struct s_db_info*)uh;

    struct s_db_cmd_seq cmd_seq[] = {
        { COMM_APD_INIT, 0, NULL},
        { COMM_APD_CMD, COMM_DB_CMD_SCHEMA, di->schema},
        { COMM_APD_CMD, COMM_DB_CMD_TABLE, di->table},
        { 0,  0, NULL}
    };


    /* pthread mutex already staticly init */
    _APDC_DB_LOCK();
    if (di->binit) {
        _APDC_DB_UNLOCK();
        return LOG_ERR;
    }
    _APDC_DB_UNLOCK();


    /* send schema and table to server */
    for (i = 0; cmd_seq[i].apd_mode != 0; i++) {

        if ( !_apd_db_comm_send(comm, COMM_CMD_MODE, cmd_seq[i].apd_mode, cmd_seq[i].command, cmd_seq[i].val) ) {
            DERR("'%s' send cmd error !\n", cmd_seq[i].val);
            return LOG_ERR;
        }

        if ( !_apd_db_comm_get_reply(comm)) {
            DERR("'%s' recieve cmd error !\n", cmd_seq[i].val);
            return LOG_ERR;
        }
    }


    /* fill 'create table' SQL command to buffer */
    if ( !generate_and_send_server_side_create_table_command(di, comm) ) {
        DERR("send create table command error !\n");
        return LOG_ERR;
    }

    if ( !_apd_db_comm_get_reply(comm) ) {
        DERR("'%s' recieve cmd erro !\n", cmd_seq[i].val);
        return LOG_ERR;
    }

    _APDC_DB_LOCK();
    di->binit = 1;
    _APDC_DB_UNLOCK();

    return LOG_OK;
}


/* description: write db appender message, return LOG_OK or LOG_ERR */
int
log_client_db_write(void *uh, struct s_msg_info *mi, struct s_comm *comm)
{
    int ret;
    struct s_db_info *di = (struct s_db_info*)uh;

    if (di == NULL)
        return 0;

    log_comm_set_header(comm, COMM_MSG_MODE, mi->msg_len, COMM_DB_APPENDER, COMM_APD_MSG);

    mi->msg_buf[COMM_HEADER_LEN] = COMM_DB_CMD_MSG;

    _APDC_DB_LOCK();
    ret = log_comm_send_raw(comm, mi->msg_len);
    _APDC_DB_UNLOCK();

    return (ret == mi->msg_len);
}


/* description: get reply, return LOG_OK or LOG_ERR */
int
log_client_db_get_reply(void *uh, struct s_comm *comm)
{
    struct s_db_info *di = (struct s_db_info*)uh;    

    if (di == NULL)
        return 0;

    return _apd_db_comm_get_reply(comm);
}


/* description: wrapper function for SQL syntax */
static void
db_field_format_string(struct s_msg_info *mi, struct s_fmt *ft, va_list va)
{
    mi->msg_buf[mi->msg_len++] = '\'';
    field_format_string(mi, ft, va);
    mi->msg_buf[mi->msg_len++] = '\'';
}


static void
db_field_format_date_and_time(struct s_msg_info *mi, struct s_fmt *ft, va_list va)
{
    mi->msg_buf[mi->msg_len++] = '\'';
    field_format_date_and_time(mi, ft, va);
    mi->msg_buf[mi->msg_len++] = '\'';
}


static void
db_field_format_msg(struct s_msg_info *mi, struct s_fmt *ft, va_list va)
{
    mi->msg_buf[mi->msg_len++] = '\'';
    field_format_msg(mi, ft, va);
    mi->msg_buf[mi->msg_len++] = '\'';
}


/* format function for deference field type */
static field_format_function dfm[] = {
    NULL,                          /* FIELD_TYPE_NONE */
    db_field_format_string,        /* FIELD_TYPE_STRING */
    db_field_format_date_and_time, /* FIELD_TYPE_DATE */
    db_field_format_date_and_time, /* FIELD_TYPE_TIME */
    db_field_format_msg,           /* FIELD_TYPE_MSG */
    NULL,                          /* FIELD_TYPE_INT */
    NULL,                          /* FIELD_TYPE_FLOAT */
};


/* description: generate SQL query string, includeing msg content and header */
int
log_client_db_format(void *uh, struct s_msg_info *mi, va_list va)
{
    struct s_fmt *ft;
    struct s_db_info *di = (struct s_db_info*)uh;

    /* first to append msg content */
    mi->msg_len = COMM_DB_HEADER_LEN;
    mi->msg_len += sprintf(&mi->msg_buf[mi->msg_len], "insert into %s values(", di->table);
    
    for (ft = di->fmt; ft; ft = ft->next) {
        if ( dfm[ ft->d_type ] ) {
            dfm[ ft->d_type ](mi, ft, va);
            
            if (ft->next)
                mi->msg_buf[mi->msg_len++] = ',';
        }
    }
    mi->msg_buf[mi->msg_len++] = ')';
    mi->msg_buf[mi->msg_len++] = ';';
    mi->msg_buf[mi->msg_len++] = '\0';

    /* DLOG("fmt %s\n", &mi->msg_buf[COMM_DB_HEADER_LEN]); */

    return LOG_OK;
}


/* description: fini db appender, check init state, then send fini
 *              command
 */
int
log_client_db_fini(void *uh, struct s_comm *comm)
{
    int ret = LOG_ERR;
    struct s_db_info *di = (struct s_db_info*)uh;

    if (di == NULL || comm == NULL)
        return LOG_ERR;

    _APDC_DB_LOCK();
    if (!di->binit) {
        _APDC_DB_UNLOCK();
        return LOG_ERR;
    }
    _APDC_DB_UNLOCK();
    
    if ( !_apd_db_comm_send(comm, COMM_CMD_MODE, COMM_APD_FINI, 0, NULL)) {
        DERR("fini send cmd error !\n");
        ret = LOG_ERR;
        goto label_exit;
    }
    ret = LOG_OK;

    _APDC_DB_LOCK();
    di->binit = 0;
    _APDC_DB_UNLOCK();

label_exit:
    return ret;
}

/* log_appender_db.c ends here */
