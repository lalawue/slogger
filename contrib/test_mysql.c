#if defined(_WIN32) || defined(_WIN64)  //�0�2�0�9�0�9�0�9�0�0��0�6�0�0windows�0�4�0�5�0�0���0�7�0�3�0�8�0�2�����0�6�0�5
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysql.h"  //�0�2�0�6�0�8�0�2�0�3���0�4�0�7�0�3�0�0�0�1�0�2�0�2�0�4�0�6�0�8�0�3/usr/local/include/mysql�0�3�0�0
 
//�0�9���0�6�0�2�0�8�0�5�0�6�0�6�0�7�0�9�0�5�0�2���0�8�0�2�0�2���0�5�0�1�0�6�0�5�0�7�0�7�0�6�0�8�0�5�0�3�0�9���0�6�0�2�0�9�0�0���0�3�0�2���0�1�0�3�0�0���0�5�0�7�0�4�0�7�0�5�0�3�0�7���0�0�0�5
/* #define SELECT_QUERY "select * from logs" */
#define SELECT_QUERY "insert into logs values('localhost', 'local4', 'err', 'haha', 'tag', '2009-11-23', '20:08:17', 'msg', 'hello, mysql', 12);"
 
int main(int argc, char **argv) //char **argv �0�3���0�8���0�7�0�3 char *argv[]
{
    MYSQL mysql,*sock;    //�0�9���0�6�0�2�0�8�0�5�0�6�0�6�0�7�0�9�0�9�0�1�0�5�0�7�0�8�0�2�0�6�0�1�����0�5�0�1�0�9�����0�3�0�7�0�1�0�7�0�3�0�4�0�0�0�2�0�1�0�9���0�7�0�4�0�8�0�2MySQL�0�2�0�4�0�8�0�5
    MYSQL_RES *res;       //�0�5���0�5�0�4�0�5���0�1�0�4�0�4�0�4�0�5�0�1�0�5���0�1�0�1�0�8���0�4�0�1
    /* MYSQL_FIELD *fd ;     //�㨹�0�2�0�1���0�0�0�9�0�2�0�4�0�3�0�3�0�4�0�8�0�2�0�5���0�1�0�1 */
    MYSQL_ROW row ;       //�0�7�0�3���0�3�0�6�0�3�0�4�0�4�0�5���0�5�0�4�0�5���0�1�0�4�0�8�0�2���0�0���0�4�0�7�0�3�0�8�0�5����
    char  qbuf[160];      //�0�7�0�3���0�3�0�5���0�5�0�4sql�0�7�0�7�0�6�0�1���0�0���0�4�0�7�0�3
    int i;
    
    if (argc != 2) {  //�0�4���0�5���0�8�0�1�0�6�0�5�0�5�0�2�0�8�0�5
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
    return 0;   //. �0�2�0�9�0�9�0�9�0�4�0�3�0�6�0�6�0�7���0�5�0�7���0�0�0�8�0�2�����0�6�0�5�0�4�0�4�0�7�0�6�0�5�0�7�0�9�0�4�0�4
}
