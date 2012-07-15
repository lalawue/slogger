/* log_appender_std.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 18:13
 * Last-Updated: 2009/12/02 09:09
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#ifndef _LOG_APPENDER_STD_H_
#define _LOG_APPENDER_STD_H_ 0

#include <stdarg.h>
#include "log_xml.h"
#include "log_comm.h"
#include "log_shared.h"

int log_std_init(void *uh, struct s_comm *comm);
int log_std_format(void *uh, struct s_msg_info *mi, va_list va);
int log_std_write(void *uh, struct s_msg_info *mi, struct s_comm *comm);
int log_std_get_reply(void *uh, struct s_comm *comm);
int log_std_fini(void *uh, struct s_comm *comm);

#endif  /* _LOG_APPENDER_STD_H_ */

/* log_appender_std.h ends here */
