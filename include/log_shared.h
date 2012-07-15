/* log_shared.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/23 14:40
 * Last-Updated: 2009/12/03 10:43
 * 
 */

/* Commentary: 
 * 
 * shared resources of server and client
 */

/* Code: */

#ifndef _LOG_SHARED_H_
#define _LOG_SHARED_H_ 0

#define LOG_OK  1
#define LOG_ERR 0

#define LOGGER_PATH "/dev/_logger"

#define LOGGER_MAX_MSG_LEN  1024 /* max message length */
#define LOGGER_MAX_PATH_LEN 256

#define	LOG_GET_MASK(pri)	(1 << (pri))		/* mask for one priority */
#define	LOG_SET_UPTO(pri)	((1 << ((pri)+1)) - 1)	/* all priorities through pri */

/* message info */
struct s_msg_info {
    struct tm *tm;              /* time */

    const char *msg_fmt;        /* message format */
    const char *name;           /* logger name */

    int pri;                    /* message priority */

    char *msg_buf;              /* message buffer */
    int msg_len;                /* message length */
    int buf_len;                /* buf length */
};

typedef void (*field_format_function)(struct s_msg_info *mi, struct s_fmt *ft, va_list va);

const char* get_pri_text(int pri);
const void* get_field_member_from_idx(struct s_msg_info *mi, struct s_fmt *ft);

void field_format_string(struct s_msg_info *mi, struct s_fmt *ft, va_list va);
void field_format_date_and_time(struct s_msg_info *mi, struct s_fmt *ft, va_list va);
void field_format_msg(struct s_msg_info *mi, struct s_fmt *ft, va_list va);


/* xml format field pair structure */
struct s_field_pair {
    char *name;
    int d_type;                /* basic data type */
    int d_size;                /* data type size, 0 means need expand */
    int m_idx;                 /* member idx */
};

/* field memeber idx value */
#define FIELD_MEMBER_IDX_NAME 0x1
#define FIELD_MEMBER_IDX_PRI  0x2
#define FIELD_MEMBER_IDX_DATE 0x3
#define FIELD_MEMBER_IDX_TIME 0x4
#define FIELD_MEMBER_IDX_MSG  0x5
#define FIELD_MEMBER_IDX_STR  0x6

struct s_field_pair* get_field_pair_from_name(const char *name);
const struct s_field_pair* get_field_pair_from_idx(int idx);


int set_non_blocking(int sock);

/* debug purpose function */
void dlog(const char *prefix, const char *fmt, ...);
void derr(const char *prefix, const char *fmt, ...);

void get_debug_level(void);

#endif  /* _LOG_SHARED_H_ */

/* log_shared.h ends here */
