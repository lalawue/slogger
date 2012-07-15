/* log_server.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/23 10:31
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
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include "log_xml.h"
#include "log_shared.h"
#include "log_comm.h"
#include "log_apd_db.h"

#define DLOG(fmt, args...) dlog("log_srv: ", fmt, ##args)
#define DERR(fmt, args...) derr("log_srv: ", fmt, ##args)

#define LOGGER_QUEUE_LENTH 10

/* server running state */
#define LOG_SERVER_STOP    0
#define LOG_SERVER_RUN     1
#define LOG_SERVER_RESTART 2

/* static alloc message buffer */
static char logger_msg_buf[LOGGER_MAX_MSG_LEN];


/* per client resources */
struct s_client_data {
    void *uh;                   /* client private handler, support 1 now */
    int idx;                    /* idx of g_apd structure */
    struct s_comm comm;         /* network comm component */
};


/* list to store client data */
struct s_client_list {
    struct s_client_data *sd;   /* server data descriptor */
    struct s_client_list *next;
};


/* server instance */
struct s_log_server {
    int run;                    /* maybe 0: stop; 1:run; 2:restart */
    char *path;                 /* UNIX domain socket path */
    int n_client;               /* client number */
    struct s_comm comm;         /* server network comm component */
    struct s_client_list *dl;   /* server data list */
};


/* server instance init state */
static struct s_log_server s_lgs = {
    .run = LOG_SERVER_RUN,
    .comm.buf = logger_msg_buf,
    .comm.buf_len = LOGGER_MAX_MSG_LEN,
};


/* server side appender definition */
typedef void* (*server_apd_init)(struct s_comm *comm);
typedef int (*server_apd_cmd)(void* user_data, char *raw_msg);
typedef int (*server_apd_reply)(struct s_comm* comm, char *content);
typedef int (*server_apd_msg)(void* user_data, char *raw_msg);
typedef void (*server_apd_fini)(void* user_data);

struct s_server_apd {
    int appender;
    server_apd_init apd_init;
    server_apd_cmd apd_cmd;
    server_apd_reply apd_reply;
    server_apd_msg apd_msg;
    server_apd_fini apd_fini;
};


/* server side definition instance */
static const struct s_server_apd
g_apd[] = {
    { COMM_DB_APPENDER
      , log_server_db_init
      , log_server_db_cmd
      , log_server_db_reply
      , log_server_db_msg
      , log_server_db_fini
    },
    { 0, NULL, NULL, NULL}
};


#define LOG_SERVER_DEFAULT_APD_IDX (-1)


/* description: malloc and init client data*/
static __inline__ struct s_client_data*
create_client_data(int con_fd, int epoll_fd, struct s_comm *scomm)
{
    struct s_client_data *sd;

    sd = (struct s_client_data*)malloc(sizeof(*sd));
    if (sd == NULL) {
        DERR("create server data error !\n");
        return NULL;
    }
    memset(sd, 0, sizeof(*sd));

    sd->comm = *scomm; /* FIXME: if we malloc a buf for every client, it will get better in log_comm_read_raw */
    sd->comm.con_fd = con_fd;
    sd->idx = LOG_SERVER_DEFAULT_APD_IDX; /* the default value, server socket will keep this */

    DLOG("create server data ...\n");

    return sd;
}


/* description: free the associated client data */
static __inline__ void
destroy_client_data(struct s_client_data *sd)
{
    if (sd) {
        free(sd);
        DLOG("destroy server data !\n");
    }
}


/* description: add client data to data_list, return the result */
static __inline__ int
add_data_entry_to_list(struct s_log_server *lgs, struct s_client_data *sd)
{
    struct s_client_list **pdl;

    if (lgs==NULL || sd==NULL)
        return 0;
    
    for (pdl = &lgs->dl; *pdl != NULL; pdl = &(*pdl)->next);

    *pdl = (struct s_client_list*)malloc(sizeof(**pdl));
    if (*pdl == NULL)
        return 0;

    memset(*pdl, 0, sizeof(**pdl));
    (*pdl)->sd = sd;

    DLOG("add data %p to list ...\n", *pdl);
        
    return 1;
}


/* description: delete data entry from list base on the given pointer,
 *              return the result
 */
static __inline__ int
del_data_entry_from_list(struct s_log_server *lgs, struct s_client_data *sd)
{
    struct s_client_list **pdl, *dlt;

    if (lgs==NULL || lgs->dl==NULL || sd==NULL)
        return 0;

    for (pdl = &lgs->dl; (*pdl != NULL) && (*pdl)->sd != sd; pdl = &(*pdl)->next);

    if (*pdl == NULL) {
        DERR("del data entry error !\n");
        return 0;
    }

    dlt = *pdl;
    if (dlt == lgs->dl)
        lgs->dl = dlt->next;
    *pdl = dlt->next;

    DLOG("del data %p from list ...\n", dlt);
    free(dlt);
    return 1;
}



/* description: pop the first entry from the data list, return NULL or
 *              entry pointer
 */
static __inline__ struct s_client_data*
pop_data_entry_from_list(struct s_log_server *lgs)
{
    struct s_client_list *dlt;
    struct s_client_data *sd;

    if (lgs == NULL || lgs->dl == NULL)
        return NULL;

    dlt = lgs->dl;
    lgs->dl = dlt->next;

    sd = dlt->sd;

    free(dlt);

    DLOG("pop data %p from list ...\n", dlt);
    return sd;
}


/* description: create the event instance, also create the client data,
 *              and add it to list
 */
static __inline__ int
add_event_instance(struct s_log_server *lgs, int epoll_fd, int con_fd, struct s_comm *scomm)
{
    struct epoll_event ev;

    set_non_blocking(con_fd);   /* set non-blocking */

    /* use data.ptr to store client data, and it will stored in kernel
     * space
     */
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = create_client_data(con_fd, epoll_fd, scomm);
    if (ev.data.ptr == NULL) {
        DERR("server create server data error !\n");
        return LOG_ERR;
    }

    /* collect the created client data pointer to our list */
    if (add_data_entry_to_list(lgs, ev.data.ptr) == LOG_ERR) {
        DERR("server add data entry to list error !\n");
        return LOG_ERR;
    }

    /* add event to epoll system */
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, con_fd, &ev) == -1) {
        DERR("server epoll ctl add error: %m\n");
        return LOG_ERR;
    }

    DLOG("add event instance ...\n");

    return LOG_OK;
}


/* description: - delete event instance
 *              - run the appender fini procedure
 *              - delete client data entry from list
 *              - close associated client socket
 *              - destroy client data
 */
static __inline__ int
del_event_instance(struct s_log_server *lgs, struct s_client_data *sd, int epoll_fd)
{
    struct epoll_event ev;

    if (sd == NULL)
        return LOG_ERR;

    ev.data.ptr = NULL;    
    ev.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sd->comm.con_fd, &ev) == -1) {
        DERR("del event instance error: %m\n");
        return LOG_ERR;
    }

    /* run appender fini procedure */
    if (sd->idx != LOG_SERVER_DEFAULT_APD_IDX)
        g_apd[sd->idx].apd_fini(sd->uh);
    
    /* delete from list before destroied */
    del_data_entry_from_list(lgs, sd);

    /* FIXME: close client socket here, nowadays we only support one
     *        appender
     */
    close(sd->comm.con_fd);

    destroy_client_data(sd);

    DLOG("delete event instance !\n");

    return LOG_OK;
}

/* description: open server's UNIX socket, set non-blocking */
static int
log_server_open(struct s_log_server *lg, char *path)
{
    struct sockaddr_un sunx;

    if (path == NULL)
        return LOG_ERR;
    lg->path = path;

    (void) unlink(path);

    sunx.sun_family = AF_UNIX;
    strncpy(sunx.sun_path, path, sizeof(sunx.sun_path));

    lg->comm.con_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (lg->comm.con_fd < 0) {
        DERR("log server: open socket error !\n");
        return LOG_ERR;
    }
    set_non_blocking(lg->comm.con_fd);

    if (bind(lg->comm.con_fd, (struct sockaddr*)&sunx, SUN_LEN(&sunx)) < 0 || chmod(lg->path, 0666) < 0) {
        DERR("bind error: %m\n");
        return LOG_ERR;
    }

    if (listen(lg->comm.con_fd, LOGGER_QUEUE_LENTH) == -1) {
        DERR("listen error: %m\n");
        return LOG_ERR;
    }

    return LOG_OK;
}


/* description: - pop client data from list remained
 *              - fini the associated server side appender
 *              - close client socket
 *              - destroy client data
 *              - close server socket
 */
static int
log_server_close(struct s_log_server *lgs, int rm_socket)
{
    struct s_client_data *sd = NULL;

    /* delete the left client data, run fini procedure  */
    while (lgs->dl) {
        sd = pop_data_entry_from_list(lgs); /* pop from the list */

        if (sd->idx != LOG_SERVER_DEFAULT_APD_IDX)
            g_apd[sd->idx].apd_fini(sd->uh);

        close(sd->comm.con_fd);     /* close client socket */
        destroy_client_data(sd); /* destroy data */
    }

    /* close server socket */
    if (lgs->comm.con_fd > 0) {
        close(lgs->comm.con_fd);
        lgs->comm.con_fd = 0;
    }

    if (lgs->path && rm_socket)
        unlink(lgs->path);

    return LOG_OK;
}


/* description: parse client message, base on the protocol, send it to
 *              proper server side appender
 */
static int
log_server_parse_msg(struct s_client_data *sd)
{
    int i, idx, appender, apd_mode;

    log_comm_get_header(&sd->comm, NULL, NULL, &appender, &apd_mode);

    /* DLOG("read %x, %x, %x\n", sd->comm.buf[0], sd->comm.buf[1], sd->comm.buf[2]); */
    /* DLOG("begin parse msg appender %d, apd_mode %d ...\n", appender, apd_mode); */

    idx = sd->idx == LOG_SERVER_DEFAULT_APD_IDX ? LOG_SERVER_DEFAULT_APD_IDX : sd->idx;

    if (idx == LOG_SERVER_DEFAULT_APD_IDX) {
        for (i = 0; g_apd[i].apd_init != NULL; i++)
            if (g_apd[i].appender == appender) {
                idx = i;
                break;
            }
    }
            
    if (idx == LOG_SERVER_DEFAULT_APD_IDX)
        return LOG_ERR;
            
    switch (apd_mode) {

        case COMM_APD_INIT:
        {
            sd->uh = g_apd[idx].apd_init(&sd->comm);
            if (sd->uh == NULL) {
                DERR("sub mode init error !\n");
                g_apd[idx].apd_reply(&sd->comm, COMM_RET_ERR);
                goto err_out;
            }

            sd->idx = idx; /* record the idx of g_apd */
            if (g_apd[idx].apd_reply(&sd->comm, COMM_RET_OK) == LOG_ERR)
                goto err_out;
            break;
        }

        case COMM_APD_CMD:
        {
            if (g_apd[idx].apd_cmd(sd->uh, sd->comm.buf) == LOG_ERR) {
                DERR("apd cmd run error !\n");
                g_apd[idx].apd_reply(&sd->comm, COMM_RET_ERR);
                goto err_out;
            }

            if (g_apd[idx].apd_reply(&sd->comm, COMM_RET_OK) == LOG_ERR)
                goto err_out;
            break;
        }
        
        case COMM_APD_MSG:
        {
            if (g_apd[idx].apd_msg(sd->uh, sd->comm.buf) == LOG_ERR) {
                DERR("apd msg run error !\n");
                g_apd[idx].apd_reply(&sd->comm, COMM_RET_ERR);
                goto err_out;
            }

            if (g_apd[idx].apd_reply(&sd->comm, COMM_RET_OK) == LOG_ERR)
                goto err_out;
            break;
        }

        case COMM_APD_FINI:
        {
            /* delete associated data in main loop, no reply */
            goto err_out;
        }

        /* server side do not deal with notice sub_mode */
        case COMM_APD_REPLY:
        default:
            goto err_out;
    } /* end switch */

    return LOG_OK;


err_out:
    return LOG_ERR;
}


#define LOGGER_CLIENT_MAX 10
#define LOGGER_EVENTS_TIMEOUT 5000 /* ms */

/* description: server main loop to accept client and recieve message */
static int
log_server_loop(struct s_log_server *lgs)
{
    struct s_client_data *sd;
    struct epoll_event events[LOGGER_CLIENT_MAX];

    int i, epoll_fd, nfds, nread;

    /* create epoll event */
    epoll_fd = epoll_create(LOGGER_CLIENT_MAX);
    if (epoll_fd < 0) {
        DERR("epoll create error: %m\n");
        return LOG_ERR;
    }

    /* add server data to epoll system, also add to list */
    if ( !add_event_instance(lgs, epoll_fd, lgs->comm.con_fd, &lgs->comm) ) {
        DERR("add server fd to event error !\n");
        return LOG_ERR;
    }


    while (lgs->run == LOG_SERVER_RUN) {

        nfds = epoll_wait(epoll_fd, events, LOGGER_CLIENT_MAX, LOGGER_EVENTS_TIMEOUT);
        if (nfds == -1) {
            DERR("server epoll wait: %m\n");
            return LOG_ERR;
        }

        /* loop event */
        for (i = 0; i < nfds; i++) {
            sd = (struct s_client_data*)events[i].data.ptr;
            if (sd == NULL)
                continue;

            /* accept client connect request */
            if (sd->comm.con_fd == lgs->comm.con_fd) {

                int con_fd = accept(lgs->comm.con_fd, NULL, NULL);
                if (con_fd == -1) {
                    DERR(" accept error: %m\n");
                    break;
                }

                if ( !add_event_instance(lgs, epoll_fd, con_fd, &lgs->comm) )
                    DERR("accept add event error !\n");
                
                DLOG("accept client %d\n", ++lgs->n_client);
            }

            /* read msg content */
            else if (events[i].events & EPOLLIN) {

                do {
                    nread = log_comm_read_raw(&sd->comm);

                    if (nread == 0)
                        break;
                    else if (nread < 0 || log_server_parse_msg(sd) == LOG_ERR)
                        goto err_read_parse;
                    
                } while (nread > 0);

                continue; /* if not a big error, continue next message */

            err_read_parse:
                del_event_instance(lgs, sd, epoll_fd);
                DLOG("disconnect client %d\n", --lgs->n_client);
            }
        }
    } /* end 'while' */

    if (epoll_fd > 0)           /* close epoll system */
        close(epoll_fd);

    return LOG_OK;
}


/* description: set state to stop when recieve a SIGTEM signal */
static void
sig_int_term_handler(int signum) 
{
    s_lgs.run = LOG_SERVER_STOP;
}


/* description: set state to restart when recieve a SIGHUP signal */
static void
sig_hug_handler(int signum)
{
    s_lgs.run = LOG_SERVER_RESTART;
}


int main(int argc, char *argv[])
{
    struct s_log_server *lgs = &s_lgs;

    /* set debug level in the very beginning */
    get_debug_level();

    /* install signal handler to recieve specific signal */
    signal(SIGINT, sig_int_term_handler);
    signal(SIGTERM, sig_int_term_handler);
    signal(SIGHUP, sig_hug_handler);

    signal(SIGPIPE, SIG_IGN);   /* ignore SIGPIPE */
    
    while (lgs->run != LOG_SERVER_STOP) {

        if (log_server_open(lgs, LOGGER_PATH) == LOG_ERR) {
            DERR("create error !\n");
            break;
        }

        if (log_server_loop(lgs) == LOG_ERR) {
            DERR("running error !\n");
            break;
        }

        log_server_close(lgs, lgs->run == LOG_SERVER_STOP);


        if (lgs->run == LOG_SERVER_RESTART) {
            DLOG("\n\n");
            DLOG("restart ...\n");
            lgs->run = LOG_SERVER_RUN;
        }
    }

    return 0;
}

/* log_server.c ends here */
