#ifndef CMN_INCLUDE
#define CMN_INCLUDE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <netdb.h>

#define HTTP_PORT_NUMBER 80

#define QUEUE_SIZE 50
#define BUFFER_SIZE 10000
#define FILE_SIZE 512

#endif // CMN_INCLUDE