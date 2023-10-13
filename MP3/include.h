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
#define QUEUE_SIZE 50
#define BUFFER_SIZE 20

#define MAX_RETRIES 10

#define TIMEOUT 4


typedef enum tftp_event_ {
    RRQ_EVENT = 1,
    WRQ_EVENT,
    DATA_EVENT,
    ACK_EVENT,
    ERR_EVENT,
	MAX_EVENT
}tftp_event_type;

typedef struct  tftp_req_{
    	uint16_t opCode;
    	char file_mode[MAX_BUF];
}tftp_req_t;


typedef struct tftp_ack_{
    	uint16_t opCode;
    	uint16_t block_num;
}tftp_ack_t;

typedef struct tftp_data_{
    	uint16_t opCode;
    	uint16_t block_num;
    	char data[MAX_BUF];
}tftp_data_t;

typedef struct tftp_err_{
    	uint16_t opCode;
    	uint16_t err_code;
    	uint8_t err_data[MAX_BUF];
}tftp_err_t;

#define NETASCII 1
#define OCTET 2