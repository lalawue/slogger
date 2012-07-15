// log_test.cpp --- 
// 
// Copyright (c) 2009 kio_34@163.com. 
// 
// Author:  suchang (kio_34@163.com)
// Maintainer: 
// 
// Created: 2009/12/02 18:59
// Last-Updated: 2009/12/03 11:18
// 
// 

// Commentary: 
// 
// 
// 

// Code:

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "log_client.h"

int
main(int argc, char *argv[])
{
    char ch;
    int fd;
    logc_t *lg, *lg1;

    if (argc != 3) {
        printf("usage: %s 1.xml 2.xml\n", argv[0]);
        return 0;
    }

    lg = log_open(argv[1]);
    lg1 = log_open(argv[2]);
    if (lg == NULL || lg1 == NULL) {
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

        if (log_emerg(lg1, "emerg testing\n") == 0) {
            printf("client: emerg error !\n");
            break;
        }

        /* sleep(1); */
        usleep(100000);
    }

    log_close(lg);
    log_close(lg1);
    printf("log close !\n");

    return 0;
}


// 
// log_test.cpp ends here
