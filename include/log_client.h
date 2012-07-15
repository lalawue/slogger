/* log_clients.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  Su Chang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/10/31 15:18
 * Last-Updated: 2009/12/02 19:01
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#ifndef _LOG_CLIENT_H_
#define _LOG_CLIENT_H_ 0

/* priority */
#define	PRI_EMERG   0           /* system is unusable */
#define	PRI_ALERT   1           /* action must be taken immediately */
#define	PRI_CRIT    2           /* critical conditions */
#define	PRI_ERR     3           /* error conditions */
#define	PRI_WARNING 4           /* warning conditions */
#define	PRI_NOTICE  5           /* normal but significant condition */
#define	PRI_INFO    6           /* informational */
#define	PRI_DEBUG   7           /* debug-level messages */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_logc logc_t;


/* open a log instance, xml should be a xml file */
logc_t* log_open(const char *xml);
void log_close(logc_t *lg);


/* write log with pri and fmt with arguments, it's thread-safe */
int log_write(logc_t *lg, int pri, const char *fmt, ...);


/* wrapper functions, you can write your own to log more, such as
 * __FILE__, __LINE__, __FUNCTION__
 */
#define log_debug(lg, fmt, args...)   log_write(lg, PRI_DEBUG, fmt, ##args)
#define log_info(lg, fmt, args...)    log_write(lg, PRI_INFO, fmt, ##args)
#define log_notice(lg, fmt, args...)  log_write(lg, PRI_NOTICE, fmt, ##args)
#define log_warning(lg, fmt, args...) log_write(lg, PRI_WARNING, fmt, ##args)
#define log_err(lg, fmt, args...)     log_write(lg, PRI_ERR, fmt, ##args)
#define log_crit(lg, fmt, args...)    log_write(lg, PRI_CRIT, fmt, ##args)
#define log_alert(lg, fmt, args...)   log_write(lg, PRI_ALERT, fmt, ##args)
#define log_emerg(lg, fmt, args...)   log_write(lg, PRI_EMERG, fmt, ##args)

#ifdef __cplusplus
}
#endif

#endif /* _LOG_CLIENT_H_ */

/* log_clients.h ends here */
