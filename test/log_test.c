/* log_test.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/26 19:25
 * Last-Updated: 2009/12/03 11:19
 * 
 */

/* Commentary: 
 * 
 * 
 */

/* Code: */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "log_client.h"

int
main(int argc, char *argv[])
{
    char ch;
    int fd;
    logc_t *lg;

    if (argc != 2) {
        printf("usage: %s 1.xml\n", argv[0]);
        return 0;
    }

    lg = log_open(argv[1]);
    if (lg == NULL) {
        printf("create logger error !\n");
        return 0;
    }

    fd = open("/dev/stdin", O_NONBLOCK);

    while (1) {
        read(fd, &ch, 1);
        if (ch == 'q')
	    break;

        if (log_info(lg, "info testing\n") == 0) {
            printf("client: info error !\n");
            break;
        }

        if (log_debug(lg, "debug testing\n") == 0) {
            printf("client: debug error !\n");
            break;
        }

        if (log_emerg(lg, "emerg testing\n") == 0) {
            printf("client: emerg error !\n");
            break;
        }

        /* sleep(1); */
        usleep(100000);
    }

    log_close(lg);
    printf("log close !\n");

    return 0;
}


/* log_test.c ends here */
