/* log_appender_db.h --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/25 18:27
 * Last-Updated: 2009/12/02 17:20
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#ifndef _LOG_APPENDER_DB_H_
#define _LOG_APPENDER_DB_H_ 0

#include <stdarg.h>
#include "log_xml.h"
#include "log_comm.h"
#include "log_shared.h"

/* db appender interfaces, including client/server side */

/* client side interface */
int log_client_db_init(void *uh, struct s_comm *comm);
int log_client_db_format(void *uh, struct s_msg_info *mi, va_list va);
int log_client_db_write(void *uh, struct s_msg_info *mi, struct s_comm *comm);
int log_client_db_get_reply(void *uh, struct s_comm *comm);
int log_client_db_fini(void *uh, struct s_comm *comm);


/* server side interface */
void* log_server_db_init(struct s_comm *comm);
int log_server_db_cmd(void *uh, char *raw_msg);
int log_server_db_reply(struct s_comm *comm, char *content);
int log_server_db_msg(void *uh, char *raw_msg);
void log_server_db_fini(void *uh);


/* protocal - base on the comm's protocal
 *
 *    0 - 6    :      7     : 8 ...
 * comm_header : db apd cmd : values
 */

#define COMM_DB_HEADER_LEN (COMM_HEADER_LEN + 1)

/* db appender command */
#define COMM_DB_CMD_SCHEMA    0x1
#define COMM_DB_CMD_TABLE     0x2
#define COMM_DB_CMD_TABLE_FMT 0x3
#define COMM_DB_CMD_NOTICE    0x4
#define COMM_DB_CMD_MSG       0x5


#define COMM_DB_APPENDER 0x01   /* name */


#endif  /* _LOG_APPENDER_DB_H_ */

/* log_appender_db.h ends here */
