/* log_appender_file.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 16:29
 * Last-Updated: 2009/12/02 09:10
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#ifndef _LOG_APPENDER_FILE_H_
#define _LOG_APPENDER_FILE_H_ 0

#include <stdarg.h>
#include "log_shared.h"

int log_file_init(void *uh, struct s_comm *comm);
int log_file_format(void *uh, struct s_msg_info *mi, va_list va);
int log_file_write(void *uh, struct s_msg_info *mi, struct s_comm *comm);
int log_file_get_reply(void *uh, struct s_comm *comm);
int log_file_fini(void *uh, struct s_comm *comm);

#endif  /* _LOG_APPENDER_FILE_H_ */

/* log_appender_file.h ends here */
