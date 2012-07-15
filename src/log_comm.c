/* log_comm.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/26 08:47
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
#include <errno.h>
#include "log_xml.h"
#include "log_shared.h"
#include "log_comm.h"

#define DLOG(fmt, args...) dlog("comm: ", fmt, ##args)
#define DERR(fmt, args...) derr("comm: ", fmt, ##args)


/* try to send in spcific times */
#define COMM_TRY_TIMES (3)


/* description: read wanted_len, and update cnt_len for next reading
 *              while encouter error cause funcion returned
 */
static __inline__ int
_log_comm_read(struct s_comm *comm, int wanted_len)
{
    int ret;
    ret = read(comm->con_fd, &comm->buf[comm->cnt_len], wanted_len);
    if (ret <= 0) {
        errno = 0;              /* reset errno */
        return 0;
    }
    else if (ret != wanted_len) {
        comm->cnt_len += ret;
        return 0;
    }
    comm->cnt_len += ret;
    return ret;
}

/* description: read a full message, also check the message length, and
 *              the fd maybe blocked or non-blocked (client/server side)
 *
 *               and every reading here is a complete message
 */
int
log_comm_read_raw(struct s_comm *comm)
{
    int msg_len, nread, ret;

    if (comm == NULL || comm->buf == NULL || comm->buf_len <= 0 || comm->con_fd <= 0) {
        DERR("read raw param error !\n");
        return 0;
    }

    /* if header not readed  */
    if (comm->cnt_len < COMM_HEADER_LEN) {
        if ((ret = _log_comm_read(comm, COMM_HEADER_LEN)) <= 0)
            return ret;
    }

    log_comm_get_header(comm, NULL, &msg_len, NULL, NULL); /* get msg len */

    /* read message part */
    if (msg_len > COMM_HEADER_LEN) {
        if ((ret = _log_comm_read(comm, (msg_len - comm->cnt_len))) <= 0)
            return ret;
    }

    comm->buf[comm->cnt_len] = '\0';

    /* DLOG("read %x, %x, %x", comm->buf[0], comm->buf[1], comm->buf[2]); */
    /* printf(", massage %s\n", &comm->buf[COMM_HEADER_LEN]); */

    nread = comm->cnt_len;
    comm->cnt_len = 0;          /* reset to 0 */
    return nread;
}


/* description: send raw data, do not modify anything, and return the
 *              bytes sended
 */
int
log_comm_send_raw(struct s_comm *comm, int msg_len)
{
    int bytes, try_times = COMM_TRY_TIMES;

    if (comm == NULL || comm->buf == NULL || comm->buf_len <= 0 || comm->con_fd <= 0 || msg_len <= 0) {
        DERR("param error !\n");
        return 0;
    }

    /* DLOG("send %x, %x, %x\n", comm->buf[0], comm->buf[1], comm->buf[2]); */

    do {
        bytes = write(comm->con_fd, comm->buf, msg_len);
        if (bytes < 0)
            DERR("comm write error: %m\n");
    } while (bytes <= 0 && errno == EAGAIN && try_times--);

    return bytes;
}


/* description: set message header with the given comm */
int
log_comm_set_header(struct s_comm *comm, int mode, int msg_len, int appender, int apd_mode)
{
    unsigned char *p;

    if (comm == NULL || comm->buf == NULL || comm->buf_len <= 0 || comm->con_fd <= 0)
        return 0;

    p = (unsigned char*)comm->buf;
    
    p[0] = mode;

    p[1] = (msg_len >> 8) & 0xff;
    p[2] = msg_len & 0xff;
    
    p[3] = (appender >> 8) & 0xff;
    p[4] = appender & 0xff;

    p[5] = (apd_mode >> 8) & 0xff;
    p[6] = apd_mode & 0xff;

    return 1;
}


/* description: get message attribute from the message buffer */
int
log_comm_get_header(struct s_comm *comm, int *mode, int *msg_len, int *appender, int *apd_mode)
{
    unsigned char *p;

    if (comm == NULL || comm->buf == NULL || comm->buf_len <= 0 || comm->con_fd <= 0)
        return 0;

    p = (unsigned char*)comm->buf;
    
    if (mode)
        *mode = p[0];

    if (msg_len) {
        *msg_len = (p[1] & 0xff) << 8;
        *msg_len |= p[2] & 0xff;
    }
    
    if (appender) {
        *appender = (p[3] & 0xff) << 8;
        *appender |= p[4] & 0xff;
    }

    if (apd_mode) {
        *apd_mode = (p[5] & 0xff) << 8;
        *apd_mode = p[6] & 0xff;
    }

    return 1;
}

/* log_comm.c ends here */
