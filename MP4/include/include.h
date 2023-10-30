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
#include <dirent.h>
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

#define MAX_NUM_CACHE 10
#define MAX_MESSAGE_LENGTH 1024

struct cache_ {
  //Relevant fields	
  char URL[256];
  char Last_Modified[50];//For Bonus
  char Access_Date[50]; 
  char Expires[50];// For Bonus
  char *content;
};

static const struct cache_ Clear_Entry;
int cache_count = 0;

struct cache_ Cached_Entries[MAX_NUM_CACHE];

int Header_parser(const char* header, char* message, char* result) {
	
	char *start = strstr(message, header);
	if(!start) {
		return 0;
	}
	char *end = strstr(start, "\r\n");
	start = start + strlen(header);
	while(*start == ' ')
		++start;
	while(*(end - 1) == ' ')
		--end;
	strncpy(result, start, end - start);
	result[end - start] = '\0';
	return 1;
}

int targetParser (char* target, char *host, int *port, char *path) {
	
	char *hostPart, *pathPart, *token;
    char *temp1, *temp2;
    char portString[16];
    
    host[0] = '\0';
    path[0] = '\0';
    *port = 0;
    
    // Check if the URL contains "http" and extract the relevant part
    if (strstr(target, "http") != NULL) {
        token = strtok(target, ":");
        temp1 = token + 7;
    } else {
        temp1 = target;
    }

    // Make a temporary copy for further parsing
    temp2 = (char *)malloc(64);
    memcpy(temp2, temp1, 64);
    
    // Check if a port is specified, otherwise use the default (80)
    if (strstr(temp1, ":") != NULL) {
        hostPart = strtok(temp1, ":");
        *port = atoi(temp1 + strlen(hostPart) + 1);
        snprintf(portString, sizeof(portString), "%d", *port);
        pathPart = temp1 + strlen(hostPart) + strlen(portString) + 1;
    } else {
        hostPart = strtok(temp1, "/");
        *port = 80;
        pathPart = temp2 + strlen(hostPart);
    }

    // Set the path to '/' if it is empty
    if (strcmp(pathPart, "") == 0) {
        strcpy(pathPart, "/");
    }

    // Copy the hostname and path to the respective output variables
    memcpy(host, hostPart, 64);
    memcpy(path, pathPart, 256);

    // Clean up the temporary copy
    free(temp2);
    
    return 0;
}

#endif // CMN_INCLUDE