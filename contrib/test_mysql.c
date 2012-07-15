#if defined(_WIN32) || defined(_WIN64)  //ÎªÁËÖ§³ÖwindowsÆ½Ì¨ÉÏµÄ±àÒë
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysql.h"  //ÎÒµÄ»úÆ÷ÉÏ¸ÃÎÄ¼þÔÚ/usr/local/include/mysqlÏÂ
 
//¶¨ÒåÊý¾Ý¿â²Ù×÷µÄºê£¬Ò²¿ÉÒÔ²»¶¨ÒåÁô×ÅºóÃæÖ±½ÓÐ´½ø´úÂë
/* #define SELECT_QUERY "select * from logs" */
#define SELECT_QUERY "insert into logs values('localhost', 'local4', 'err', 'haha', 'tag', '2009-11-23', '20:08:17', 'msg', 'hello, mysql', 12);"
 
int main(int argc, char **argv) //char **argv Ïàµ±ÓÚ char *argv[]
{
    MYSQL mysql,*sock;    //¶¨ÒåÊý¾Ý¿âÁ¬½ÓµÄ¾ä±ú£¬Ëü±»ÓÃÓÚ¼¸ºõËùÓÐµÄMySQLº¯Êý
    MYSQL_RES *res;       //²éÑ¯½á¹û¼¯£¬½á¹¹ÀàÐÍ
    /* MYSQL_FIELD *fd ;     //°üº¬×Ö¶ÎÐÅÏ¢µÄ½á¹¹ */
    MYSQL_ROW row ;       //´æ·ÅÒ»ÐÐ²éÑ¯½á¹ûµÄ×Ö·û´®Êý×é
    char  qbuf[160];      //´æ·Å²éÑ¯sqlÓï¾ä×Ö·û´®
    int i;
    
    if (argc != 2) {  //¼ì²éÊäÈë²ÎÊý
        fprintf(stderr,"usage : mysql_select <userid>\n\n");
        exit(1);
    }
    
    mysql_init(&mysql);
    if (!(sock = mysql_real_connect(&mysql, "localhost", "rsyslog","rsyslog","syslog", 0, NULL, 0))) {
        fprintf(stderr,"Couldn't connect to engine!\n%s\n\n",mysql_error(&mysql));
        perror("");
        exit(1);
    }
    
    /* sprintf(qbuf, SELECT_QUERY, atoi(argv[1])); */
    sprintf(qbuf, SELECT_QUERY);
    if(mysql_query(sock,qbuf)) {
        fprintf(stderr, "Query failed (%s)\n", mysql_error(sock));
        exit(1);
    }
    
    if (!(res = mysql_store_result(sock))) {
        fprintf(stderr, "Couldn't get result from %s\n", mysql_error(sock));
        exit(1);
    }
    
    printf("number of fields returned: %d\n", mysql_num_fields(res));
        
    while ((row = mysql_fetch_row(res))) {
        for (i = 0; i < mysql_num_fields(res); i++)
            printf("result is: %s\n", (((row[i]==NULL)&&(!strlen(row[0]))) ? "NULL" : row[i]));
        puts( "query ok !\n" );
    }
    
    mysql_free_result(res);
    mysql_close(sock);
    exit(0);
    return 0;   //. ÎªÁË¼æÈÝ´ó²¿·ÖµÄ±àÒëÆ÷¼ÓÈë´ËÐÐ
}
