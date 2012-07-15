/* log_xml.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 09:56
 * Last-Updated: 2009/12/03 09:23
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#ifndef _LOG_XML_H_
#define _LOG_XML_H_ 0


/* appender format structure */
struct s_fmt {
    int d_type;                 /* field data type */
    int d_size;                 /* field data size */
    int m_idx;                  /* appender member idx */
    void *val;                  /* field value */
    struct s_fmt *next;
};


struct s_std_info {
    int fd;
    struct s_fmt *fmt;
};


/* file appender instance info */
struct s_file_info {
    char *path;                 /* path */
    int n_type;                 /* file name type */
    char *n_val;                /* file name value */
    int r_type;                 /* rolling type */
    int r_val;                  /* rolling value */

    char *name;                 /* logger name */
    FILE *fp;                   /* file handler */
    long long size;             /* file size, bytes */

    struct s_fmt *fmt;          /* file entry format */
};


/* db appender instance info */
struct s_db_info {
    char *schema;               /* schema name */
    char *table;                /* table name */
    struct s_fmt *fmt;          /* db entry format */

    void *mutex;                /* pthread mutex */
    int binit;                  /* binit */
};


/* appender instance */
struct s_appender {
    int type;                   /* appender type */
    int pri_mask;               /* or level */
    int network;                /* network support */
    void *info;                 /* info of file/db/std, depends on type */
    struct s_appender *next;
};


/* logger instance */
struct s_logger {
    char *name;
    struct s_appender *apd;
};


/* appender type */
#define APD_TYPE_NONE   0
#define APD_TYPE_STDOUT 1
#define APD_TYPE_STDERR 2
#define APD_TYPE_FILE   3
#define APD_TYPE_DB     4


/* file path type */
#define LOG_FILE_DEFAULT_PATH ("/var/log/Jabsco/")


/* file name type */
#define FILE_NAME_TYPE_NONE   0 /* invalid */
#define FILE_NAME_TYPE_LOGGER 1 /* logger name */
#define FILE_NAME_TYPE_DATE   2 /* date/time format */
#define FILE_NAME_TYPE_NAME   3 /* normal name */

#define FILE_NAME_VAL_DATE ("%Y-%m") /* default date/time format */


/* file rolling type */
#define FILE_ROLLING_TYPE_NONE 0 /* invalid */
#define FILE_ROLLING_TYPE_DATE 1 /* rolling by date, not implement */
#define FILE_ROLLING_TYPE_SIZE 2 /* rolling by size */


/* fmt field basic data type */
#define XML_FIELD_TYPE_NONE   0 /* invalid */

#define XML_FIELD_TYPE_STRING 1 /* string */
#define XML_FIELD_TYPE_DATE   2 /* date */
#define XML_FIELD_TYPE_TIME   3 /* time */
#define XML_FIELD_TYPE_TEXT   4 /* text */
#define XML_FIELD_TYPE_INT    5 /* int */
#define XML_FIELD_TYPE_FLOAT  6 /* float */


/* public functions */
struct s_logger* logc_create_logger(const char *xml);
void logc_destroy_logger(struct s_logger *lgr);

/* description: print out all the information */
void logc_xml_test_logger(struct s_logger *lgr);

#endif  /* _LOG_XML_H_ */

/* log_xml.h ends here */
