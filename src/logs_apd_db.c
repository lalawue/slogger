/* logs_appender_db.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/26 15:37
 * Last-Updated: 2009/12/02 19:39
 * 
 */

/* Commentary: 
 * 
 * server side database appender
 */

/* Code: */

/*
 * below is db appender server side 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "log_comm.h"
#include "log_apd_db.h"


#define DLOG(fmt, args...) dlog("apds_db: ", fmt, ##args)
#define DERR(fmt, args...) derr("apds_db: ", fmt, ##args)


/* server side db appender state */
#define _APDS_DB_DISCONNECT 0
#define _APDS_DB_INIT_OK    1
#define _APDS_DB_SCHEMA_OK  (1<<1)
#define _APDS_DB_TABLE_OK   (1<<2)


/* server side db appender structure */
struct s_db_server {
    MYSQL mysql;
    MYSQL *sock;
    int status;              /* disconnect; init_ok; schema_ok; table_ok */
    struct s_comm *comm;
};

#define LOGGER_ENV_USER_KEYWORD "SLOG_USER"
#define LOGGER_ENV_PASSWORD_KEYWORD "SLOG_PASSWORD"


/* description: init and connect db */
static int
_apd_db_init_and_connect(struct s_db_server *ds)
{
    char *user, *passwd;

    if (ds->status != _APDS_DB_DISCONNECT) {
        DERR("init param error !\n");
        return 0;
    }

    user = getenv(LOGGER_ENV_USER_KEYWORD);
    passwd = getenv(LOGGER_ENV_PASSWORD_KEYWORD);

    if (user == NULL || passwd == NULL) {
        DERR("user or password error !\n");
        return 0;
    }

    mysql_init(&ds->mysql);

    ds->sock = mysql_real_connect(&ds->mysql, "localhost", user, passwd, NULL, 0, NULL, 0);
    if (ds->sock == NULL) {
        DERR("connect db error: %s\n", mysql_error(&ds->mysql));
        goto err_connect;
    }

    ds->status |= _APDS_DB_INIT_OK;
    DLOG("init and connect db ok ...\n");

    return 1;
    
err_connect:
    mysql_close(&ds->mysql);
    ds->status = _APDS_DB_DISCONNECT;
    return 0;
}


/* description: disconnect and close db */
static int
_apd_db_disconnect_and_close(struct s_db_server *ds)
{
    if (ds == NULL || ds->status == _APDS_DB_DISCONNECT) {
        DERR("discoonect param error ds %p, status %d, sock %p\n", ds, ds->status, ds->sock);
        return 0;
    }

    mysql_close(ds->sock);
    ds->sock = NULL;
    ds->status = _APDS_DB_DISCONNECT;
    
    return 0;
}


/* description: query db */
static __inline__ int
_apd_db_query(struct s_db_server *ds, const char *fmt, const char *val)
{
    char command[LOGGER_MAX_PATH_LEN];

    sprintf(command, fmt, val);
    if ( mysql_query(ds->sock, command) ) {
        DERR("query '%s' error: %s\n", command, mysql_error(ds->sock));
        return 0;
    }
    /* free the result  */
    {
        MYSQL_RES *res = mysql_store_result(ds->sock);
        mysql_free_result(res);
    }
    return 1;
}


/* description: use/create schema */
static int
_apd_db_use_or_create_schema(struct s_db_server *ds, const char *schema)
{
    if (ds->sock == NULL) {
        DERR("mysql sock error !\n");
        return 0;
    }
    
    while ( !_apd_db_query(ds, "use %s;", schema) ) {
        if ( !_apd_db_query(ds, "create database %s;", schema) ) {
            return 0;
        }
    }

    ds->status |= _APDS_DB_SCHEMA_OK;
    DLOG("create schema ok ...\n");
    return 1;
}


/* description: show table status */
int
_apd_db_show_table(struct s_db_server *ds, const char *table)
{
    int ret;
    if (ds->sock == NULL) {
        DERR("mysql sock error !\n");
        return 0;
    }

    ret = _apd_db_query(ds, "show create table %s;", table);

    if (ret) {
        ds->status |= _APDS_DB_TABLE_OK;
        DLOG("show table ok ...\n");
    }

    return ret;
}


/* description: crate table */
int
_apd_db_create_table(struct s_db_server *ds, const char *create_tlb)
{
    if (ds->sock == NULL) {
        DERR("mysql sock error !\n");
        return 0;
    }

    if ( !_apd_db_query(ds, "%s;", create_tlb) ) {
        return 0;
    }

    ds->status |= _APDS_DB_TABLE_OK;
    DLOG("create table ok ...\n");
    return 1;
}


/* description: create server side db appender instance */
void*
log_server_db_init(struct s_comm *comm)
{
    struct s_db_server *ds;

    DLOG("begin init ...\n");

    ds = (struct s_db_server*)malloc(sizeof(*ds));
    if (ds == NULL)
        return NULL;

    memset(ds, 0, sizeof(*ds));

    ds->comm = comm;

    DLOG("init ok ...\n");

    return ds;
}


/* description: close db connection on the server side */
void
log_server_db_fini(void *uh)
{
    struct s_db_server *ds = (struct s_db_server*)uh;

    if ( ds ) {

        _apd_db_disconnect_and_close(ds);

        free(ds);
    }
}


/* description: reply client */
int
log_server_db_reply(struct s_comm *comm, char *content)
{
    int ret, msg_len = COMM_DB_HEADER_LEN;

    msg_len += sprintf(&comm->buf[msg_len], "%s", content);

    log_comm_set_header(comm, COMM_CMD_MODE, msg_len, COMM_DB_APPENDER, COMM_APD_REPLY);
    comm->buf[COMM_HEADER_LEN] = COMM_DB_CMD_NOTICE;

    ret = log_comm_send_raw(comm, msg_len);

    /* DLOG("send reply %s\n", content); */
    return (ret == msg_len);
}


/* check db init state */
#define _IS_APD_DB_INIT_OK(S)                   \
    (((S) & _APDS_DB_INIT_OK)                   \
     && ((S) & _APDS_DB_SCHEMA_OK)              \
     && ((S) & _APDS_DB_TABLE_OK))


/* description: respond client request */
int
log_server_db_cmd(void *uh, char *raw_cmd)
{
    unsigned char mode;

    mode = (unsigned char)raw_cmd[0];

    /* DLOG("run in mode %c\n", info->mode); */
    
    if (mode == COMM_CMD_MODE) {
        int ret = 1;
        char *val, command;

        struct s_db_server *ds = (struct s_db_server*)uh;

        if (ds == NULL || raw_cmd == NULL)
            return 0;

        command = raw_cmd[COMM_HEADER_LEN];
        val = &raw_cmd[COMM_DB_HEADER_LEN];

        switch (command) {

            case COMM_DB_CMD_SCHEMA:
                ret &= _apd_db_init_and_connect(ds);
                if (ds->status & _APDS_DB_INIT_OK)
                    ret &= _apd_db_use_or_create_schema(ds, val);
                break;

            case COMM_DB_CMD_TABLE: 
                if (ds->status & _APDS_DB_SCHEMA_OK)
                    _apd_db_show_table(ds, val);
                /* return ok here */
                break;


            case COMM_DB_CMD_TABLE_FMT:
                if ( !(ds->status & _APDS_DB_TABLE_OK) )
                    ret = _apd_db_create_table(ds, val);
                break;

            default:
                ret = 0;
                break;
        }

        return ret;
    }

    DLOG("!!! should not reach here !\n\n");
    
    return 0;
}


/* description: collect client message */
int
log_server_db_msg(void *uh, char *raw_msg)
{
    unsigned char mode;

    mode = (unsigned char)raw_msg[0];

    if (mode == COMM_MSG_MODE) {
        char *msg;
        struct s_db_server *ds = (struct s_db_server*)uh;
    
        if (ds == NULL || ds->sock == NULL || !_IS_APD_DB_INIT_OK(ds->status) )
            return 0;

        msg = &raw_msg[COMM_DB_HEADER_LEN];
        /* DLOG("db msg content __'%s'__\n", msg); */

        if ( mysql_query(ds->sock, msg) ) {
            DERR("msg '%s' error: '%s'\n", msg, mysql_error(ds->sock));
            return 0;
        }
    
        return 1;
    }

    DLOG("!!! should not reach here !\n\n");
    
    return 0;
}


/* logs_appender_db.c ends here */
