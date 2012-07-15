/* log_comm.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/26 08:47
 * Last-Updated: 2009/12/02 17:16
 * 
 */

/* Commentary: 
 * 
 * network communication component
 */

/* Code: */

#ifndef _LOG_COMM_H_
#define _LOG_COMM_H_ 0

/* 
 * message syntax
 *
 * 0 :   12    :    34    :    56    : 7 ...
 * m : msg_len : appender : apd_mode : rest ...
 *
 * byte
 * 0    : message mode
 * 1 - 2: message length
 * 3 - 4: appender
 * 5 - 6: appender mode
 * 7 -  : rest part
 */

/* 0: message mode */
#define COMM_CMD_MODE 'c'    /* command message */
#define COMM_MSG_MODE 'm'    /* normal message */

/* comm part */
struct s_comm {
    int con_fd;
    char *buf;
    int buf_len;                /* avaliable buf length */
    int cnt_len;                /* readed content length */
};

#define COMM_HEADER_LEN 7       /* cmd and msg header len */

/* the comm header just part of s_comm's buf */
struct s_comm_head {

    unsigned char mode;

    unsigned char len_1;
    unsigned char len_2;

    unsigned char appender_1;
    unsigned char appender_2;

    unsigned char apd_mode_1;
    unsigned char apd_mode_2;
};


/* public interfaces */

int log_comm_get_header(struct s_comm *comm, int *mode, int *msg_len, int *appender, int *apd_mode);
int log_comm_set_header(struct s_comm *comm, int mode, int msg_len, int appender, int apd_mode);

int log_comm_read_raw(struct s_comm *comm);
int log_comm_send_raw(struct s_comm *comm, int msg_len);


/* comm appender command */
#define COMM_APD_INIT  0x1
#define COMM_APD_CMD   0x2      /* need reply */
#define COMM_APD_REPLY 0x3
#define COMM_APD_MSG   0x4      /* no reply */
#define COMM_APD_FINI  0x5

#define COMM_RET_OK  "ok"
#define COMM_RET_ERR "err"

#endif  /* _LOG_COMM_H_ */

/* log_comm.h ends here */
