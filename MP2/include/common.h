#ifndef CMN_INCLUDE
#define CMN_INCLUDE

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#define QUEUE_SIZE 50
#define BUFFER_SIZE 20
#define PAYLOAD_SIZE 512

/* SBCP Protocol Version */
#define PROTOCOL_VERSION 3

/* Header type */
#define JOIN 2
#define SEND 4
#define FWD 3
#define ACK 7
#define NAK 5
#define ONLINE 8
#define OFFLINE 6
#define IDLE 30

/* Stdin File descriptor */
#define STDIN_FD 0

/* Attributes type */
#define REASON 1
#define USERNAME 2
#define CLIENT_COUNT 3
#define MESSAGE 4

#define IDLE_WAIT_TIME 10

/* SBCP message header format */
typedef struct sbcp_header_
{
    unsigned int uiVrsn:9;
    unsigned int uiType:7;
    unsigned int uiLength:16;
}sbcp_header_t;

/* SBCP message attribute formate */
typedef struct sbcp_attribute_
{
    unsigned int uiType:16;
    unsigned int uiLength:16;
    char         acPayload[PAYLOAD_SIZE];
}sbcp_attribute_t;

/* SBCP message structure */
typedef struct sbcp_message_
{
    sbcp_header_t    sMsgHeader;
    sbcp_attribute_t sMsgAttribute;
}sbcp_message_t;

// User data
struct user_data{
    char user_name[16];
    int socket_fd;
    int user_number;
};

#endif // CMN_INCLUDE
