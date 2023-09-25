#ifndef CMN_INCLUDE
#define CMN_INCLUDE

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <netinet/in.h>
#include <errno.h>

#define QUEUE_SIZE 50       // Max Queue Size
#define BUFFER_SIZE 50      // Max Buffer Size
#define MAX_CLIENT_SIZE 100 //Maximum clients on the server
// Function decleration
int server_sock_init(struct sockaddr_in *sockaddr, char *ip_addr, int port);
ssize_t readLine(int fd, void *vptr, size_t maxlen); 
ssize_t writen(int fd, void *buf_ptr, size_t bufSize);
void read_write(int connect_fd);
int send_data(int sockfd, char *data);
int receive_data(int sockfd, char *buffer, int buf_size);

#endif // CMN_INCLUDE
