/* log_misc_test.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/12/02 14:56
 * Last-Updated: 2009/12/02 15:30
 * 
 */

/* Commentary: 
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "log_client.h"

int
main(int argc, char *argv[])
{
    logc_t *lg;

    int i, fd;
    char ch, msg_buf[512];

    lg = log_open(argv[1]);

    fd = open("/dev/stdin", O_NONBLOCK);

    memset(msg_buf, 0, sizeof(msg_buf));
    for (i = 0; i < sizeof(msg_buf) - 1; i++) {
        msg_buf[i] = 'a' + i%24;
    }

    while (1) {
        read(fd, &ch, 1);
        if (ch == 'q')
	    break;

        if (log_info(lg, "%s\n", msg_buf) == 0) {
            printf("client: info error !\n");
            break;
        }

        /* if (log_debug(lg, "debug testing\n") == 0) { */
        /*     printf("client: debug error !\n"); */
        /*     break; */
        /* } */

        /* if (log_emerg(lg, "emerg testing\n") == 0) { */
        /*     printf("client: emerg error !\n"); */
        /*     break; */
        /* } */

        /* sleep(1); */
        /* usleep(1); */
    }

    log_close(lg);
    printf("log close !\n");

    return 0;
}


/* Code: */




/* log_misc_test.c ends here */
