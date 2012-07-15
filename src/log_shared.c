/* log_shared.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 17:50
 * Last-Updated: 2009/12/03 10:45
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
#include <fcntl.h>
#include <time.h>
#include "log_xml.h"
#include "log_shared.h"

#define DLOG(fmt, args...) dlog("log_sht: ", fmt, ##args)
#define DERR(fmt, args...) derr("log_sht: ", fmt, ##args)

int g_debug;                    /* global debug mode indicator */

/* priority text, the sequence was defined in log_client.h */
static const char
*log_pri_text[] = {
    "emerg",
    "alert",
    "crit",
    "err",
    "warning",
    "notice",
    "info",
    "debug"
};

/* description: get priority text */
const char *
get_pri_text(int pri)
{
    if ((pri >= 0) && (pri < sizeof(log_pri_text)/sizeof(log_pri_text[0])))
        return log_pri_text[pri];

    return NULL;
}

/* description: refers to 'struct s_field_pair' in log_xml.c */
const void*
get_field_member_from_idx(struct s_msg_info *mi, struct s_fmt *ft)
{
    switch (ft->m_idx) {
        case FIELD_MEMBER_IDX_NAME: /* name */
            return mi->name;
        case FIELD_MEMBER_IDX_PRI: /* priority */
            return get_pri_text(mi->pri);
    }
    return NULL;
}

/* description: basic formater for specific type */
void
field_format_string(struct s_msg_info *mi, struct s_fmt *ft, va_list va_msg)
{
    char *format = (char*)ft->val;
    mi->msg_len += sprintf((char*)&mi->msg_buf[mi->msg_len], format, (char*)get_field_member_from_idx(mi, ft));
    /* mi->len += printf(format, (char*)get_field_member_from_idx(lg, mi, ft)); */
}

void
field_format_date_and_time(struct s_msg_info *mi, struct s_fmt *ft, va_list va_msg)
{
    mi->msg_len += strftime((char*)&mi->msg_buf[mi->msg_len], LOGGER_MAX_MSG_LEN - mi->msg_len, (char*)ft->val, mi->tm);
    /* printf("%s, len %d\n", ft->val, strlen(ft->val)); */
}

void
field_format_msg(struct s_msg_info *mi, struct s_fmt *ft, va_list va_msg)
{
    /* printf("\n\n\n%s\n\n\n", (char*)ft->val); */
    mi->msg_len += sprintf((char*)&mi->msg_buf[mi->msg_len], (char*)ft->val);
    mi->msg_len += vsprintf((char*)&mi->msg_buf[mi->msg_len], mi->msg_fmt, va_msg);
    /* mi->len += printf((char*)ft->val); */
    /* mi->len += vprintf(mi->msg_fmt, va); */
}

/* format field pair instance */
static struct s_field_pair
g_fr[] = {
    { "logger"                  /* logger name */
      , XML_FIELD_TYPE_STRING
      , 0
      , FIELD_MEMBER_IDX_NAME   /* appender member idx */
    }, 
    { "priority" 
      , XML_FIELD_TYPE_STRING
      , 8                       /* refers to 'log_pri_text' */
      , FIELD_MEMBER_IDX_PRI
    },     
    { "date"
      , XML_FIELD_TYPE_DATE
      , 0                       /* we will determine it later */
      , FIELD_MEMBER_IDX_DATE
    },
    { "time" 
      , XML_FIELD_TYPE_TIME
      , 0
      , FIELD_MEMBER_IDX_TIME
    },
    { "message"
      , XML_FIELD_TYPE_TEXT
      , LOGGER_MAX_MSG_LEN - 1
      , FIELD_MEMBER_IDX_MSG
    },
    { "text"
      , XML_FIELD_TYPE_STRING
      , 0
      , FIELD_MEMBER_IDX_STR
    },
    { NULL, 0}
};


/* description: get field pair from field name */
struct s_field_pair*
get_field_pair_from_name(const char *name)
{
    int i;
    for (i = 0; g_fr[i].name != NULL; i++) {
        if ( !strcmp(g_fr[i].name, name) )
            return &g_fr[i];
    }

    return NULL;
}

/* description: get field pair from m_idx value */
const struct s_field_pair*
get_field_pair_from_idx(int idx)
{
    int i;
    for (i = 0; g_fr[i].name != NULL; i++) {
        if (g_fr[i].m_idx == idx)
            return &g_fr[i];
    }

    return NULL;
}

/* description: set non-block on the given socket */
int
set_non_blocking(int sock)
{
    int opts;

    opts = fcntl(sock , F_GETFL);
    if(opts < 0) {
        DERR("fcntl(sock,GETFL) error: %m\n");
        return LOG_ERR;
    }

    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        DERR("fcntl(sock,SETFL,opts): %m\n");
        return LOG_ERR;
    }

    return LOG_OK;
}

/* description: get debug level from enviroment, invoke like 'export
 *              SLOG_DEBUG=1'
 */
void
get_debug_level(void)
{
    char *p;
    extern int g_debug;

    p = getenv("SLOG_DEBUG");

    if (p && !strcmp(p, "1"))
        g_debug = 1;
}


/* description: print log to stdout with prefix string */
void
dlog(const char *prefix, const char *fmt, ...)
{
    va_list va;

    if (g_debug == 0 || prefix == NULL || fmt == NULL)
        return;

    fprintf(stdout, "%s", prefix);

    va_start(va, fmt);
    vfprintf(stdout, fmt, va);
    va_end(va);
}

/* description: print log to stderr with prefix string */
void
derr(const char *prefix, const char *fmt, ...)
{
    va_list va;

    if (g_debug == 0 || prefix == NULL || fmt == NULL)
        return;

    fprintf(stdout, "%s", prefix);

    va_start(va, fmt);
    vfprintf(stdout, fmt, va);
    va_end(va);
}

/* log_shared.c ends here */
