#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <wait.h>
#include <errno.h>

#define MAX_BUF 512

#define BUFSIZE 20

#define RETRY_LIMIT 10

#define TIMEOUT 4

#define MIN_FRAME_LEN 4


typedef enum tftp_event_ {
    RRQ_EVENT = 1,
    WRQ_EVENT,
    DATA_EVENT,
    ACK_EVENT,
    ERR_EVENT,
	MAX_EVENT
}tftp_event_type;

typedef union mess_recv mess_recv_t;
union mess_recv {
	
	uint16_t opCode;
	
	struct {
    	uint16_t opCode;
    	char file_mode[MAX_BUF];
	}tftp_req;

	struct {
    	uint16_t opCode;
    	uint16_t block_num;
	}tftp_ack;

	struct {
    	uint16_t opCode;
    	uint16_t block_num;
    	char data[MAX_BUF];
	}tftp_data;

	struct {
    	uint16_t opCode;
    	uint16_t err_code;
    	uint8_t err_data[MAX_BUF];
	}tftp_err;
};	

typedef enum error_event_ {
  UNDEFINED = 0,
  FILE_NOT_FOUND,
  ACCESS_VIOLATION,
  DISK_FULL,
  ILLEGAL_OP,
  UNKNOWN_TID,
  FILE_EXISTS,
  NO_SUCH_USER,
  MAX_ERROR
} error_event_t;

#define NETASCII 1
#define OCTET 2
