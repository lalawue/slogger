/* log_xml.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/24 10:46
 * Last-Updated: 2009/12/03 10:44
 * 
 */

/* Commentary: 
 * 
 * read configure file of log_client
 */

/* Code: */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <expat.h>
#include "log_client.h"
#include "log_xml.h"
#include "log_shared.h"

#define DLOG(fmt, args...) dlog("xml: ", fmt, ##args)
#define DERR(fmt, args...) derr("xml: ", fmt, ##args)


/* parser handler */
struct s_parser {
    struct s_appender *cur_ah;  /* current appender */
    struct s_logger *lgr;
};


/* description: create logger element */
static int
logc_xml_create_logger(void *uh, const char **attrs)
{
    int i = 0;
    struct s_logger *lgx;
    struct s_parser *ph = (struct s_parser*)uh;

    /* check format */
    if ( strcmp(attrs[i++], "name") ) {
        DERR("logger invalid format !\n");
        return LOG_ERR;
    }

    if ( attrs[i] == NULL) {
        DERR("logger no name provide !\n");
        return LOG_ERR;
    }

    /* malloc logger instance */
    lgx = (struct s_logger*)malloc(sizeof(*lgx));
    if (lgx == NULL) {
        DERR("logger malloc error !\n");
        return LOG_ERR;
    }
    memset(lgx, 0, sizeof(*lgx));

    lgx->name = strdup(attrs[i]);
    if (lgx->name == NULL) {
        DERR("logger strdup name error !\n");
        goto err_malloc;
    }

    ph->lgr = lgx;
    
    return LOG_OK;

err_malloc:
    free(lgx);
    return LOG_ERR;
}


/* description: set default file info*/
static void
logc_init_file_info(struct s_logger *lgx, struct s_file_info *fi)
{
    fi->path = LOG_FILE_DEFAULT_PATH;
    fi->n_type = FILE_NAME_TYPE_LOGGER;
    fi->n_val = strdup(lgx->name);
    fi->r_type = FILE_ROLLING_TYPE_NONE;
    fi->r_val = 0;              /* kbytes */
    fi->name = lgx->name;

    /* set default format in the end element tag */
}


/* appender (string, type) pair */
struct s_appender_str_type {
    char *name;
    int type;
    int network;                /* network support */
};

static struct s_appender_str_type
g_apd_st[] = {
    { "stdout", APD_TYPE_STDOUT, 0},
    { "stderr", APD_TYPE_STDERR, 0},
    { "file", APD_TYPE_FILE, 0},
    { "database", APD_TYPE_DB, 1},
    { NULL, 0, 0}
};


/* description: get appender (string, type) pair with appender name */
static struct s_appender_str_type*
logc_get_appender_str_type(const char *name)
{
    int i;

    for (i = 0; g_apd_st[i].name != NULL; i++)
        if ( !strcmp(g_apd_st[i].name, name) )
            return &g_apd_st[i];

    return NULL;
}


/* description: create appender element */
static int
logc_xml_create_appender(void *uh, const char **attrs)
{
    int i = 0;
    const char *name;

    struct s_appender *ah, **pah;
    struct s_appender_str_type *ast;
    struct s_parser *ph = (struct s_parser*)uh;

    /* check format */
    if ( strcmp(attrs[i++], "value") ) {
        DERR("appender format error !\n");
        return LOG_ERR;
    }

    name = attrs[i];
    if (name == NULL) {
        DERR("appender did not provide name !\n");
        return LOG_ERR;
    }

    if ((ast = logc_get_appender_str_type(name)) == NULL) {
        DERR("appender error type: %s\n", name);
        return LOG_ERR;
    }


    /* create appender instance */
    ah = (struct s_appender*)malloc(sizeof(*ah));
    if (ah == NULL) {
        DERR("appender malloc error !\n");
        return LOG_ERR;
    }
    memset(ah, 0, sizeof(*ah));

    ah->type = ast->type;
    ah->network = ast->network;
    ah->pri_mask = LOG_SET_UPTO(PRI_DEBUG); /* default logs every thing */


    /* create appender actual info */
    if (ah->type == APD_TYPE_FILE) {

        ah->info = (struct s_file_info*)malloc(sizeof(struct s_file_info));

        if (ah->info == NULL) {
            DERR("appender file info malloc error !\n");
            goto err_info;
        }

        logc_init_file_info(ph->lgr, ah->info); /* init file info */
    }
    else if (ah->type == APD_TYPE_DB) {

        ah->info = (struct s_db_info*)malloc(sizeof(struct s_db_info));

        if (ah->info == NULL) {
            DERR("appender db info malloc error !\n");
            goto err_info;
        }
        memset(ah->info, 0, sizeof(struct s_db_info));
    }

    /* APD_TYPE_STDOUT or APD_TYPE_STDERR */
    else {
        ah->info = (struct s_std_info*)malloc(sizeof(struct s_std_info));
        
        if (ah->info == NULL) {
            DERR("appender std info malloc error !\n");
            goto err_info;
        }
        memset(ah->info, 0, sizeof(struct s_std_info));

        ((struct s_std_info*)ah->info)->fd = ah->type; /* set fd */
    }


    /* add it to logger apd list */
    for (pah = &ph->lgr->apd; *pah != NULL; pah = &(*pah)->next);
    *pah = ah;
    ph->cur_ah = ah;            /* fill in current appender */
    
    return LOG_OK;

err_info:
    free(ah);
    return LOG_ERR;
}


#define PRI_NONE   (PRI_DEBUG + 1)

/* description: set appender priority or level value*/
static int 
logc_set_pri_or_level(void *uh, const char **attrs, int upto)
{
    int i, pri;
    const char *pri_text, *p;

    struct s_parser *ph = (struct s_parser*)uh;
    
    /* check format */
    i = 0;
    if ( strcmp(attrs[i++], "value") ) {
        DERR("level format error !\n");
        return LOG_ERR;
    }
    pri_text = attrs[i];

    /* validate pri */
    pri = PRI_NONE;
    for (i = 0; pri_text && (p = get_pri_text(i)); i++)
        if ( p && !strcmp(p, pri_text) )
            pri = i;
            
    if (pri == PRI_NONE) {
        DERR("level value error !\n");
        return LOG_ERR;
    }

    /* set the value to the current appender */
    ph->cur_ah->pri_mask = upto ? LOG_SET_UPTO(pri) : LOG_GET_MASK(pri);

    return LOG_OK;
}


/* description: priority wrapper */
static __inline__ int
logc_xml_set_level(void *uh, const char **attrs)
{
    return logc_set_pri_or_level(uh, attrs, 0);
}

/* description: level wrapper */
static __inline__ int
logc_xml_set_priority(void *uh, const char **attrs)
{
    return logc_set_pri_or_level(uh, attrs, 1);
}

/* description: set path element */
static int
logc_xml_set_path(void *uh, const char **attrs)
{
    int i;
    const char *path;

    struct s_file_info *fi;
    struct s_parser *ph = (struct s_parser*)uh;


    if (ph->cur_ah->type != APD_TYPE_FILE) {
        DERR("path illegal setting !\n");
        return LOG_ERR;
    }

    /* check format */
    i = 0;
    if ( strcmp(attrs[i++], "value") ) {
        DERR("path format error !\n");
        return LOG_ERR;
    }
    path = attrs[i];

    fi = (struct s_file_info*)ph->cur_ah->info;
    fi->path = strdup(path);
    if (fi->path == NULL) {
        DERR("path strdup error !\n");
        return LOG_ERR;
    }
    
    ph->cur_ah->info = (void*)fi;
    
    return LOG_OK;
}


/* filename (string, type) pair */
struct s_file_name_type {
    char *name;
    int type;
};

static struct s_file_name_type
g_ft[] = {
    { "logger", FILE_NAME_TYPE_LOGGER},
    { "date", FILE_NAME_TYPE_DATE},
    { "name", FILE_NAME_TYPE_NAME},
    {NULL, 0}
};

/* description: set filename element */
static int
logc_xml_set_filename(void *uh, const char **attrs)
{
    int i, type;
    const char *name, *val;

    struct s_file_info *fi;
    struct s_parser *ph = (struct s_parser*)uh;

    if (ph->cur_ah->type != APD_TYPE_FILE) {
        DERR("path illegal setting !\n");
        return LOG_ERR;
    }

    /* check format */
    for (i = 0; attrs[i]; i++);

    if (i < 3 || strcmp(attrs[0], "type") || strcmp(attrs[2], "value") ) {
        DERR("filename format error !\n");
        return LOG_ERR;
    }

    name = attrs[1];
    val = attrs[3];

    type = FILE_NAME_TYPE_NONE;
    for (i = 0; g_ft[i].name != NULL; i++)
        if ( !strcmp(g_ft[i].name, name) ) {
            type = g_ft[i].type;
            break;
        }

    if (type == FILE_NAME_TYPE_NONE) {
        DERR("filename type error !\n");
        return LOG_ERR;
    }

    /* FILE_NAME_TYPE_DATE: are expanded when openning file */
    fi = (struct s_file_info*)ph->cur_ah->info;
    fi->n_type = type;

    switch (type) {
        case FILE_NAME_TYPE_DATE:
        case FILE_NAME_TYPE_LOGGER:
        case FILE_NAME_TYPE_NAME:
            fi->n_val = strdup(val);
            break;
    }
    

    if (fi->n_val == NULL) {
        DERR("filename strdup error !\n");
        return LOG_ERR;
    }

    return LOG_OK;
}

/* file rolling type (string, type) pair */
struct s_file_rolling_type {
    char *name;
    int type;
};

static struct s_file_rolling_type
g_rt[] = {
    { "date", FILE_ROLLING_TYPE_DATE},
    { "size", FILE_ROLLING_TYPE_SIZE},
    {NULL, 0}
};


/* description: set file rolling element */
static int
logc_xml_set_rolling(void *uh, const char **attrs)
{
    int i, type;
    const char *name, *val;

    struct s_file_info *fi;
    struct s_parser *ph = (struct s_parser*)uh;    


    if (ph->cur_ah->type != APD_TYPE_FILE) {
        DERR("path illegal setting !\n");
        return LOG_ERR;
    }


    /* check format */
    for (i = 0; attrs[i]; i++);

    if (i < 3 || strcmp(attrs[0], "type") || strcmp(attrs[2], "value") ) {
        DERR("rolling format error !\n");
        return LOG_ERR;
    }

    name = attrs[1];
    val = attrs[3];

    type = FILE_ROLLING_TYPE_NONE;
    for (i = 0; g_rt[i].name != NULL; i++)
        if ( !strcmp(g_rt[i].name, name) )
            type = g_rt[i].type;

    if (type == FILE_ROLLING_TYPE_NONE) {
        DERR("rolling format error !\n");
        return LOG_ERR;
    }

    fi = (struct s_file_info*)ph->cur_ah->info;
    fi->r_type = type;
    fi->r_val = atoi(val);

    return LOG_OK;
}

/* description: expand date/time string, and get the expanded size */
static __inline__ int
_get_date_time_string_size(const char *fmt)
{
    time_t now;
    struct tm tms;
    
    int fmt_len;
    char time_buf[LOGGER_MAX_PATH_LEN];
    
    time(&now);
    localtime_r(&now, &tms);

    /* set max value avoid lack space */
    tms.tm_sec = 19;
    tms.tm_min = 19;
    tms.tm_hour = 19;
    tms.tm_mday = 19;
    tms.tm_mon = 11;
    tms.tm_year = 1999;
    tms.tm_wday = 3;
    tms.tm_yday = 256;

    fmt_len = strftime(time_buf, LOGGER_MAX_PATH_LEN, fmt, &tms);
    /* printf("%s\n", time_buf); */
    return fmt_len;
}

/* description: get normal string size from the given m_idx */
static __inline__ int
_get_norm_string_size(struct s_parser *ph, const char *fmt_val, int m_idx)
{
    int fmt_len = 0;
    char fmt_buf[LOGGER_MAX_PATH_LEN], *val = NULL;

    switch (m_idx) {
        case FIELD_MEMBER_IDX_NAME:
            val = ph->lgr->name;
            if (val)
                fmt_len = sprintf(fmt_buf, fmt_val, val);
            break;

        case FIELD_MEMBER_IDX_STR:
            fmt_len = strlen(fmt_val);
            break;

        default:
            break;
    }

    return fmt_len;
}

/* description: creaet format field element */
static int
logc_xml_create_field(void *uh, const char **attrs)
{
    int i, local_error = 0;
    const char *fmt_val = NULL;

    struct s_fmt **pfmt = NULL;
    struct s_field_pair *fr = NULL;
    struct s_parser *ph = (struct s_parser*)uh;

    if (ph->cur_ah == NULL || ph->cur_ah->type==APD_TYPE_NONE) {
        DERR("field appender error !\n");
    }
    
    /* check format */
    for (i = 0; attrs[i]; i++) {

        if (i & 1)
            continue;

        if ( !strcmp(attrs[i], "name") ) {
            if (!attrs[i+1] || !(fr = get_field_pair_from_name(attrs[i+1])))
                local_error = 1;
        }
        else if ( !strcmp(attrs[i], "format") ) {
            if (!attrs[i+1])
                local_error = 1;
            else
                fmt_val = attrs[i+1];
        }
        else {
            local_error = 1;
        }
    }

    if (i < 3 || local_error) {
        DERR("field format error !\n");
        return LOG_ERR;
    }

    /* get type size, expand names if needed */
    switch (fr->d_type) {
        case XML_FIELD_TYPE_STRING:
            fr->d_size = fr->d_size ? fr->d_size : _get_norm_string_size(ph, fmt_val, fr->m_idx);
            break;

        case XML_FIELD_TYPE_DATE:
        case XML_FIELD_TYPE_TIME:
            fr->d_size = fr->d_size ? fr->d_size : _get_date_time_string_size(fmt_val);
            break;
    }

    /* get appender's last fmt pointer */
    if (ph->cur_ah->type == APD_TYPE_STDOUT || ph->cur_ah->type == APD_TYPE_STDERR) {
        struct s_std_info *info = (struct s_std_info*)ph->cur_ah->info;

        for (pfmt = &info->fmt; *pfmt != NULL; pfmt = &(*pfmt)->next);
    }
    else if (ph->cur_ah->type == APD_TYPE_FILE) {
        struct s_file_info *info = (struct s_file_info*)ph->cur_ah->info;

        for (pfmt = &info->fmt; *pfmt != NULL; pfmt = &(*pfmt)->next);
    }
    else if (ph->cur_ah->type == APD_TYPE_DB) {
        struct s_db_info *info = (struct s_db_info*)ph->cur_ah->info;

        for (pfmt = &info->fmt; *pfmt != NULL; pfmt = &(*pfmt)->next);
    }

    *pfmt = (struct s_fmt*)malloc(sizeof(struct s_fmt));
    if (*pfmt == NULL) {
        DERR("field malloc error !\n");
        return LOG_ERR;
    }
    memset(*pfmt, 0, sizeof(**pfmt));


    /* assign value now */
    (*pfmt)->d_type = fr->d_type;
    (*pfmt)->d_size = fr->d_size;
    (*pfmt)->m_idx = fr->m_idx;


    switch (fr->d_type) {
        case XML_FIELD_TYPE_STRING:
        case XML_FIELD_TYPE_DATE:
        case XML_FIELD_TYPE_TIME:
        case XML_FIELD_TYPE_TEXT:
            (*pfmt)->val = fmt_val ? strdup(fmt_val) : NULL;
            break;
            
        case XML_FIELD_TYPE_INT:
            (*pfmt)->val = malloc(fr->d_size);
            *((int*)(*pfmt)->val) = atoi(fmt_val);
            break;

        case XML_FIELD_TYPE_FLOAT:
            (*pfmt)->val = malloc(fr->d_size);
            *((float*)(*pfmt)->val) = atof(fmt_val);
            break;

        default:
            break;
    }

    if (fmt_val && (*pfmt)->val == NULL) {
        DERR("field malloc val error !\n");
        return LOG_ERR;
    }

    return LOG_OK;
}

/* description: create table/schema element template */
#define LOGC_XML_CREATE_TABLE_OR_SCHEMA(UH, ATTRS, M, STR)              \
    do {                                                                \
        const char **attrs = ATTRS;                                     \
                                                                        \
        struct s_parser *ph = (struct s_parser*)UH;                     \
        struct s_db_info *di = (struct s_db_info*)ph->cur_ah->info;     \
                                                                        \
        if (ph->cur_ah->type != APD_TYPE_DB || di == NULL) {            \
            DERR("%s illegal setting !\n", STR);                        \
            return LOG_ERR;                                             \
        }                                                               \
                                                                        \
        if (attrs[0]==NULL || attrs[1]==NULL                            \
            || strcmp(attrs[0], "name") ) {                             \
                                                                        \
            DERR("%s format error !\n", STR);                           \
            return LOG_ERR;                                             \
        }                                                               \
                                                                        \
        di->M = strdup(attrs[1]);                                       \
        if (di->M == NULL) {                                            \
            DERR("%s strdup error !\n", STR);                           \
            return LOG_ERR;                                             \
        }                                                               \
                                                                        \
        return LOG_OK;                                                  \
    } while (0)

/* description: wrapper to create table element */
static int
logc_xml_create_table(void *uh, const char **s_attrs)
{
    LOGC_XML_CREATE_TABLE_OR_SCHEMA(uh, s_attrs, table, "table");
}

/* description: wrapper to create schema element */
static int
logc_xml_create_schama(void *uh, const char **s_attrs)
{
    LOGC_XML_CREATE_TABLE_OR_SCHEMA(uh, s_attrs, schema, "schema");
}

/* description: destroy format field element */
static void
logc_distroy_fmt(struct s_fmt *fmt)
{
    struct s_fmt *fmt_next;
    if (fmt) {
        /* printf("free fmt !\n"); */
        fmt_next = fmt->next;
        if (fmt->val)
            free(fmt->val);
        free(fmt);
        logc_distroy_fmt(fmt_next);
    }
}

/* destroy: destroy appender element */
static void
logc_distroy_appender(struct s_appender *apd)
{
    struct s_appender *apd_next;

    if (apd) {
        /* printf("free apd !\n"); */
        apd_next = apd->next;
        switch (apd->type) {
            case APD_TYPE_STDERR:
            case APD_TYPE_STDOUT:
            {
                struct s_std_info *si = (struct s_std_info*)apd->info;
                if (si)
                    logc_distroy_fmt(si->fmt);
                break;
            }

            case APD_TYPE_FILE:
            {
                struct s_file_info *fi = (struct s_file_info*)apd->info;
                if (fi) {
                    if (fi->path != LOG_FILE_DEFAULT_PATH)
                        free(fi->path);
                    if (fi->n_val)
                        free(fi->n_val);
                    logc_distroy_fmt(fi->fmt);
                }
                break;
            }

            case APD_TYPE_DB: 
            {
                struct s_db_info *di = (struct s_db_info*)apd->info;
                if (di) {
                    if (di->schema)
                        free(di->schema);
                    if (di->table)
                        free(di->table);
                    logc_distroy_fmt(di->fmt);
                }

                break;
            }
        }

        if (apd->info)
            free(apd->info);
        free(apd);
        logc_distroy_appender(apd_next);
    }
}

/* description: destroy logger element */
void
logc_destroy_logger(struct s_logger *lgr) 
{
    if (lgr) {
        /* printf("free logger !\n"); */
        if (lgr->name)
            free(lgr->name);
        logc_distroy_appender(lgr->apd);
        free(lgr);
    }
}


typedef int (*logc_conf_callback)(void *ph, const char **attrs);

struct s_element {
    char *name;
    const logc_conf_callback cb;
    int result;
};

static struct s_element
g_element[] = {
    { "logger",  
      logc_xml_create_logger,
      0},
    { "appender",
      logc_xml_create_appender,
      0},
    { "level",
      logc_xml_set_level,
      0},
    { "priority",
      logc_xml_set_priority,
      0},
    { "path",
      logc_xml_set_path,
      0},
    { "filename",
      logc_xml_set_filename,
      0},
    { "rolling",
      logc_xml_set_rolling,
      0},
    { "field",
      logc_xml_create_field,
      0},
    { "table",
      logc_xml_create_table,
      0},
    { "schema",
      logc_xml_create_schama,
      0},
    { NULL, NULL, 0}
};

/* description: start element parser, callback function */
static void
logc_xml_start_element(void *uh, const char *name, const char **attrs)
{
    int i;
    struct s_element *eh = g_element;

    for (i = 0; eh[i].name != NULL; i++) {
        if ( !strcmp(eh[i].name, name) )
            eh[i].result = eh[i].cb(uh, attrs);
    }
}

/* default format field value */
static const char
*g_default_fmt[][5] = {
    { "name", "priority", "format", "%s.", NULL},
    { "name", "date", "format", " %y/%m/%d", NULL},
    { "name", "time", "format", " %H:%M", NULL},
    { "name", "name", "format", " %s", NULL},
    { "name", "message", "format", ": ", NULL},
    { NULL}
};


/* description: set the default formater */
static __inline__ void
_fill_default_fmt_format(void *uh)
{
    int i;
    const char **attrs;

    for (i = 0; g_default_fmt[i][0] != NULL; i++) {
        attrs = g_default_fmt[i];
        logc_xml_create_field(uh, attrs);
    }
}

/* description: set the default filename */
static __inline__ void
_fill_default_filename(void *uh)
{
    struct s_parser *ph = (struct s_parser*)uh;    
    struct s_file_info *info = (struct s_file_info*)ph->cur_ah->info;

    info->n_type = FILE_NAME_TYPE_LOGGER;
    info->n_val = strdup(ph->lgr->name);
}

/* description: fill default value for appender */
static void
logc_xml_end_element(void *uh, const char *name)
{
    struct s_parser *ph = (struct s_parser*)uh;

    if ( strcmp(name, "appender") )
        return;

    switch (ph->cur_ah->type) {
        case APD_TYPE_STDOUT:
        case APD_TYPE_STDERR:
        {
            struct s_std_info *info = (struct s_std_info*)ph->cur_ah->info;
            if (info->fmt == NULL)
                goto label_fill_default_format;
            break;
        }

        case APD_TYPE_FILE:
        {
            struct s_file_info *info = (struct s_file_info*)ph->cur_ah->info;
            if (info->fmt == NULL)
                goto label_fill_default_format;

            if (info->n_type == FILE_NAME_TYPE_NONE)
                goto label_fill_default_filename;
            break;
        }

        case APD_TYPE_DB:
        {
            struct s_db_info *info = (struct s_db_info*)ph->cur_ah->info;
            if (info->fmt == NULL)
                goto label_fill_default_format;
            break;
        }
        
        default:
            break;
    }

    return;


    if (0) {
    label_fill_default_format:
        _fill_default_fmt_format(uh);
    }
    else if (0) {
    label_fill_default_filename:
        _fill_default_filename(uh);
    }
}


#define XML_PARSER_BUF_SIZE 512 /* parser buffer size */

/* description: create logger instance */
struct s_logger*
logc_create_logger(const char *xml)
{
    FILE *fp;
    int len, done;

    XML_Parser xph;
    struct s_parser ph;

    char buf[XML_PARSER_BUF_SIZE];


    if (xml == NULL) {
        DERR("paramenter error !\n");
        return NULL;
    }

    if ((fp = fopen(xml, "r")) == NULL) {
        DERR("open conf file error !\n");        
        return NULL;
    }


    xph = XML_ParserCreate(NULL);
    if (xph == NULL) {
        DERR("create parser error !\n");
        goto err_file;
    }

    XML_SetUserData(xph, &ph);
    XML_SetElementHandler(xph, logc_xml_start_element, logc_xml_end_element);

    do {
        len = fread(buf, 1, sizeof(buf), fp);
        done = len < sizeof(buf);

        if (XML_Parse(xph, buf, len, done) != XML_STATUS_OK) {

            DERR("xml parser error %s at line %ld\n",
                 XML_ErrorString(XML_GetErrorCode(xph)),
                 XML_GetCurrentLineNumber(xph));

            goto err_parse;
        }
    } while (!done);

    XML_ParserFree(xph);
    fclose(fp);

    return ph.lgr;

    
err_parse:
    XML_ParserFree(xph);
err_file:
    fclose(fp);
    logc_destroy_logger(ph.lgr);
    return NULL;
}

/* description: function for debug purpose */
static void 
logc_xml_test_fmt(struct s_fmt *ft)
{
    const struct s_field_pair *fr;

    if (ft) {
        fr = get_field_pair_from_idx(ft->m_idx);
        printf("<field type=\"%s\", size=\"%d\"", fr ? fr->name : NULL, ft->d_size);
        switch (ft->d_type) {
            case XML_FIELD_TYPE_STRING:
            case XML_FIELD_TYPE_DATE:
            case XML_FIELD_TYPE_TIME:
            default:
                if (ft->val)
                    printf(", format=\"%s\" />\n", (char*)ft->val);
                else
                    printf(" />\n");
        }
        logc_xml_test_fmt(ft->next);
    }
}

static __inline__ int
logc_xml_test_get_pri_from_mask(int pri_mask)
{
    int pri, mask;

    pri = PRI_DEBUG;
    mask = 1<<PRI_DEBUG;

    while (pri_mask && mask && pri) {

        if (pri_mask & mask)
            break;

        pri--;
        mask >>= 1;
    }

    return pri;
}

void
logc_xml_test_logger(struct s_logger *lgr)
{
    struct s_appender *apd;

    if (!lgr)
        return;
    
    printf("<logger name=\"%s\">\n", lgr->name);

    for (apd = lgr->apd; apd; apd = apd->next) {
        printf("\n");
        printf("<appender value=\"%s\">\n", g_apd_st[apd->type - 1].name);
        printf("<priority value=\"%s\"/>\n", get_pri_text(logc_xml_test_get_pri_from_mask(apd->pri_mask)));

        
        if (apd->type == APD_TYPE_STDOUT || apd->type == APD_TYPE_STDERR) {
            struct s_std_info *si = (struct s_std_info*)apd->info;
            logc_xml_test_fmt(si->fmt);
        }
        else if (apd->type == APD_TYPE_FILE) {
            struct s_file_info *fi = (struct s_file_info*)apd->info;
            printf("<path value=\"%s\"/>\n", fi->path);
            printf("<filename type=\"%s\", value=\"%s\"/>\n", g_ft[fi->n_type - 1].name, fi->n_val);
            printf("<rolling type=\"%s\", value=\"%d\"/>\n", g_rt[fi->r_type - 1].name, fi->r_val);
            logc_xml_test_fmt(fi->fmt);
        }
        else if (apd->type == APD_TYPE_DB) {
            struct s_db_info *di = (struct s_db_info*)apd->info;
            printf("<schema name=\"%s\"/>\n", di->schema);
            printf("<table name=\"%s\"/>\n", di->table);
            logc_xml_test_fmt(di->fmt);
        }
        printf("</appender>\n");
    }

    printf("</logger>\n");
}

#if 0
int main(int argc, char *argv[])
{
    struct s_logger *lgr;
    lgr = logc_create_logger(argv[1]);
    if (lgr == NULL)
        return 0;

    logc_xml_test_logger(lgr);

    logc_destroy_logger(lgr);

    return 0;
}
#endif

/* log_xml.c ends here */
