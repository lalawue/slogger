/* log_test.c --- 
 * 
 * Copyright (c) 2009 kio_34@163.com. 
 * 
 * Author:  suchang (kio_34@163.com)
 * Maintainer: 
 * 
 * Created: 2009/11/26 19:25
 * Last-Updated: 2009/12/02 14:33
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
#include <pthread.h>
#include "log_client.h"

static int g_thread_run = 1;

static void*
thread_run(void *vparam)
{
    logc_t *lg = (logc_t*)vparam;

    while (g_thread_run) {

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
    }

    pthread_exit(NULL);
}


int
main(int argc, char *argv[])
{
    char ch;
    int i, fd, thread_num = 30;

    logc_t *lg;
    pthread_t pt[thread_num];

    lg = log_open(argv[1]);

    fd = open("/dev/stdin", O_NONBLOCK);

    for (i = 0; i < thread_num; i++) {
        pthread_create(&pt[i], NULL, thread_run, lg);
    }
    

    while (1) {
        read(fd, &ch, 1);
        if (ch == 'q')
	    break;

        /* sleep(1); */
        usleep(100000);
    }

    g_thread_run = 0;

    for (i = 0; i < thread_num; i++) {
        pthread_join(pt[i], NULL);
    }

    log_close(lg);
    printf("log close !\n");

    return 0;
}


/* log_test.c ends here */
