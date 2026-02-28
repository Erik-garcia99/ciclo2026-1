#ifndef TCP_LIB_H
#define TCP_LIB_H


#define SERVER_PORT 80
#define BUFFER_LENGTH 256
#define FALSE 0
#define HOST "worldclockapi.com"
#define REQUEST "GET /api/json/pst/now HTTP/1.1\r\nHost: worldclockapi.com\r\nConnection: close\r\n\r\n"

char *get_time_tcp(void);



#endif