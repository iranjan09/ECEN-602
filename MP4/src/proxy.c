#define _XOPEN_SOURCE /* See feature_test_macros(7) */
#include <time.h>
#include "include.h"

//Print all cache entries
int Dump_Cache() {
	
    int index;
	//No entries in cache
    if (cache_count == 0) {
		printf("Cache is currently empty.\n");
    } else {
		//Can display total entries
        printf("Cache Entries:\n");
		//loop through the entries
        for (index = 0; index < cache_count; index++) {
            printf("Cache Entry %d\n", index + 1);
            printf("URL: %s\n", Cached_Entries[index].URL);
            printf("Access Date: %s\n", Cached_Entries[index].Access_Date);
            //if present then only access, otherwise crashes
            if (strcmp(Cached_Entries[index].Expires, "") != 0) {
                printf("Expires: %s\n", Cached_Entries[index].Expires);
            } else {
                printf("Expires: N/A\n");
            }
            
            if (strcmp(Cached_Entries[index].Last_Modified, "") != 0) {
                printf("Last Modified: %s\n", Cached_Entries[index].Last_Modified);
            } else {
                printf("Last Modified: N/A\n");
            }
            
            printf("\n");
        }
    }
	return 0;
}
//Todo: Function long, maybe have two separate functions one for add and other for update.
int UpdateCacheEntry(char *target, char *info, int newEntry, int entry) {
	
	int entries = 0;
	//check for new entry flag
	if (newEntry == 1) {
		//Max cache count reached, clear first entry
		if (cache_count == MAX_NUM_CACHE) {
			Cached_Entries[0] = Clear_Entry;
			for (entries = 0; entries < MAX_NUM_CACHE; entries++){
				//Shift the entries
				if (entries + 1 != MAX_NUM_CACHE)
					Cached_Entries[entries] = Cached_Entries[entries+1];
				else {
					// New entry add to head
					memset(&Cached_Entries[entries], 0, sizeof(struct cache_));
					memcpy(Cached_Entries[entries].URL, target, 256);
					Cached_Entries[entries].content = (char *) malloc(strlen(info));
					memcpy(Cached_Entries[entries].content, info, strlen(info));
					//look for the fields and copy
					Header_parser("Expires:", info, Cached_Entries[entries].Expires);
					//TODO: check if present
					Header_parser("Last-Modified:", info, Cached_Entries[entries].Last_Modified);
					Header_parser("AccessDate:", info, Cached_Entries[entries].Access_Date);
				}
			}
		} else {
			//Space is there just add the entry
			Cached_Entries[cache_count] = Clear_Entry;
			memcpy(Cached_Entries[cache_count].URL, target, 256); 
			Cached_Entries[cache_count].content = (char *) malloc(strlen(info));
			memcpy(Cached_Entries[cache_count].content, info, strlen(info));
			//Todo: Same as above
			Header_parser("Expires:", info, Cached_Entries[cache_count].Expires);
			Header_parser("Last-Modified:", info, Cached_Entries[cache_count].Last_Modified);
			Header_parser("Date:", info, Cached_Entries[cache_count].Access_Date);
			//Increment the count
			cache_count++;
		}
	} else {
		//Entry already present just update it it
		struct cache_ temp_entry;
		memset(&temp_entry, 0, sizeof(struct cache_));
		//reach the entry
		temp_entry = Cached_Entries[entry];
		//Walk through the entries
		for (entries = entry; entries < cache_count; entries++) {
			if (entries == cache_count - 1)
				break;
			//Shift them
			Cached_Entries[entries] = Cached_Entries[entries + 1];
		}
		Cached_Entries[cache_count -1] = temp_entry;
		//Update the time with current time
		struct tm current_time;
		time_t now = time(NULL); 
		//get current time
		current_time = *gmtime(&now);
		// From Man
		const char *date_format = "%a, %d %b %Y %H:%M:%S GMT";
		//Change current time in Access
		strftime(Cached_Entries[cache_count - 1].Access_Date, sizeof(Cached_Entries[cache_count - 1].Access_Date), date_format, &current_time);	
	}
	return 0;
}

//Serach based on URL     
int Cache_FindElement(char *target) {
	
	int entry = 0;
	//Walk throigh the entries
	for (entry = 0; entry < MAX_NUM_CACHE; entry++) {
		if (strcmp(Cached_Entries[entry].URL, target) == 0) {
			//Found entry, return its index
			return entry;
		}
	}
	
	return -1;
}

int IsCacheEntryFresh (int entry) {
	
	time_t currentTime = time(NULL);
	//get current time in GMT
    struct tm currentGMTTime;
	currentGMTTime = *gmtime(&currentTime);
    struct tm expiresTime;

    if (strcmp(Cached_Entries[entry].Expires, "") != 0) {
		//From Man
        strptime(Cached_Entries[entry].Expires, "%a, %d %b %Y %H:%M:%S %Z", &expiresTime);
        //Todo:Date cannot be parsed Add print maybe   
        //conver the time to time_t to compare
        time_t expires = mktime(&expiresTime);
		//From https://pubs.opengroup.org/onlinepubs/009695399/functions/mktime.html
        time_t now = mktime(&currentGMTTime);
        
        //Not expired, entry should be fresh
        if (difftime(now, expires) < 0) {
            return 1;
        } else
			return -1;
    } else {
		//Expired entry return -1, set variable
		return -1; 
	}
}	

//Connect to remote host
int connectHost(char *hostname) {
	
	int websocket = -1, s = -1;
	struct addrinfo hints, *result, *rp;
	
	//From getaddrinfo Man page
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM;
	//ToDo: set family as unspecific, check
	if ((s = getaddrinfo(hostname, "http", &hints, &result)) != 0) {
		fprintf(stderr, "Server: getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	
	for(rp = result; rp != NULL; rp = rp->ai_next) {
		//Create socket
		websocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		//If seuccesful connect and break, otherwise keep trying
		if (websocket >= 0 && (connect(websocket, rp->ai_addr, rp->ai_addrlen) >= 0)) 
			break;
	}
	//No socket found
	if (rp == NULL)
		websocket = -1;
	//Todo: Check if any assigned, might cause crash
	freeaddrinfo(result);
	return websocket;
}

//Parse response from remote page
int parseMessageBody(int sockfd, char *messageBuffer) {
	
    int totalBytesRead = 0;
	//init to 1
    int bytesRead = 1;
    char dataBuffer[MAX_MESSAGE_LENGTH] = {0};
	
    while (bytesRead > 0) {
        memset(dataBuffer, 0, sizeof(dataBuffer));
		//Start reading the socket
        bytesRead = read(sockfd, dataBuffer, MAX_MESSAGE_LENGTH);

        if (bytesRead <= 0) {
            //end of read or error,break
            break;
        }

        // Add data to received buffer
        strcat(messageBuffer, dataBuffer);
        totalBytesRead += bytesRead;

        //REached EoF break, remove NULL part 
        if (dataBuffer[bytesRead - 1] == EOF) {
			//ToDo: Use strncpy. ---DONE
			strncpy(messageBuffer,messageBuffer,(strlen(messageBuffer)-1));
            totalBytesRead--;
            break;
        }
    }

    return totalBytesRead;
}

int start_proxy(int sockfd) {
	
	char *recv_buf;
	char reqtype[8] = {0}, target[256] = {0}, httpVersion[16] = {0};
	int cachedEntry, isNotExpired = -1;
	char* dataToSend = NULL, *response = NULL;
	char host[64], path[256], urlSegment[256] = {0};
	int websocket, wport =80;
	char msg_gen[256] = {0};
	int bytesRead = 0;
	char client_req[MAX_MESSAGE_LENGTH] = {0};
  
	recv_buf = (char *)calloc(MAX_MESSAGE_LENGTH, sizeof(char));

	//Read the socket, free the memory	
	if (read(sockfd, recv_buf, MAX_MESSAGE_LENGTH) < 0) {
		perror("Server Error: Unable to read meessage from client");
		free(recv_buf);
	}
  
	printf("Server: Received request for page:\n%s", recv_buf);
 	//Parse the request
	sscanf(recv_buf, "%s %s %s", reqtype, target, httpVersion);

	//Check if entry already present
	cachedEntry = Cache_FindElement(target);
	//If entry present, is it expired
	if (cachedEntry != -1)
		isNotExpired = IsCacheEntryFresh(cachedEntry);

	//if entry present and not expired just update
	if (cachedEntry != -1 && isNotExpired == 1) {
		printf("Server: Client request for url: %s is present in cache and is not expired\n", target);
    
		// Just update the entry with new access time
		UpdateCacheEntry(target, NULL, 0, cachedEntry);
    
		// Calloc memory to send to client
		dataToSend = (char*)calloc(strlen(Cached_Entries[cachedEntry].content) + 1, sizeof(char));
    
		// Copy the content
		memcpy(dataToSend, Cached_Entries[cachedEntry].content, strlen(Cached_Entries[cachedEntry].content));
	} else {
        //Entry not cached or expired
		memset(path, 0, 256);
		memset(host, 0, 64);  
		//Parse the URL
		memcpy(&urlSegment[0], &target[0], 256);
		targetParser(urlSegment, host, &wport, path);
		//Connect to remote server
		//Todo: free recv_buf
		if ((websocket = connectHost (host)) == -1)
			perror ("Server ERROR: Proxy cannot connect to host");
		
		printf ("Server: Connected successfully to remote server %d\n", websocket);
		
		if (cachedEntry != -1) {
			
			// Entry cached but expired
			printf ("Server: URL %s in cache has expired.\n", target);
			
			char ifModifiedSince[60];
			//Check for modificatons
			//From the MP4 problem
			if (strcmp(Cached_Entries[cachedEntry].Expires, "") != 0 && strcmp(Cached_Entries[cachedEntry].Last_Modified, "") != 0)
				snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified_Since: %s", Cached_Entries[cachedEntry].Expires);
			else if (strcmp(Cached_Entries[cachedEntry].Expires, "") == 0 && strcmp(Cached_Entries[cachedEntry].Last_Modified, "") == 0) 
				snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified_Since: %s", Cached_Entries[cachedEntry].Access_Date);
			else if (strcmp(Cached_Entries[cachedEntry].Expires, "") == 0)
				snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified_Since: %s", Cached_Entries[cachedEntry].Last_Modified);
			else if (strcmp(Cached_Entries[cachedEntry].Last_Modified, "") == 0) 
				snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified_Since: %s", Cached_Entries[cachedEntry].Expires);
			//gENEERATE the msg to  be sent
			snprintf(msg_gen, MAX_MESSAGE_LENGTH, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\n%s\r\n\r\n", reqtype, path, httpVersion, host, ifModifiedSince);
			
			printf("GET Generated: \n%s", msg_gen);
			//write to socket
			write(websocket, msg_gen, MAX_MESSAGE_LENGTH); 
			//Todo: calloc might be better
			response = (char *) malloc (100000);
			//parse response from webserver
			bytesRead = parseMessageBody(websocket, response);
			//Todo:same as above
			dataToSend = (char *) malloc(strlen(response));

			//page same, just send cache entry
			if (strstr(response, "304 Not Modified") != NULL) {
				//ToDo: print it.---DONE
				printf("Received '304 Not Modified' response. Sending cached file instead.\n");
				memcpy(dataToSend, Cached_Entries[cachedEntry].content, strlen(Cached_Entries[cachedEntry].content)); 
				//just update access time
				UpdateCacheEntry(target, NULL, 0, cachedEntry);
			} else {
				//add print.---DONE
				printf("Server: File stored in cache was modified, update it\n");
				//update data in cache
				memcpy(dataToSend, response, strlen(response)); 
				UpdateCacheEntry(target, NULL, 0, cachedEntry);
				//clear last entry
				Cached_Entries[--cache_count] = Clear_Entry;
				// now treat as new one
				//Todo: use a flag for new entry, 1, 0 is confusing
				UpdateCacheEntry(target, response, 1, 0);     
			}
		}else {
			//Target not in cached space get it 
			printf ("Server: Client requested url is not present in cache. Fetch it.\n");
			//just utilize client request
			memset(client_req, 0, MAX_MESSAGE_LENGTH);
			snprintf(client_req, MAX_MESSAGE_LENGTH, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\n\r\n", reqtype, path, httpVersion, host);
			printf("Server: Generated Request from Client: \n%s", client_req);
			//ask for the page
			write(websocket, client_req, MAX_MESSAGE_LENGTH); 
			//Todo: Use calloc
			response = (char *) malloc (100000);
			//Parse to get the data			
			bytesRead = parseMessageBody(websocket, response);
			//Todo:same
			dataToSend = (char *) malloc(strlen(response));
			memcpy(dataToSend, response, strlen(response));
			//neew entry add to cache
			UpdateCacheEntry(target, response, 1, 0);
		}
	}
	//Print all in cache
	Dump_Cache();
	//send to client
	write(sockfd, dataToSend, strlen(dataToSend) + 1);
	return 0;
}

int main (int argc, char *argv[]) {
	
    if (argc != 3){
		//Arguments less than expected
		printf("Proxy ERROR: Please provide Proxy Server IP address and port number\n");
		printf("Usage: ./proxy <Proxy IP> <Port>");
        exit(EXIT_FAILURE);
	}
	
	char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
	struct sockaddr_in servaddr;
	socklen_t socklen;
	int listen_fd, new_fd;
	struct sockaddr_storage client_addr; 
	socklen = sizeof(client_addr);
	pthread_t thread;
	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_fd < 0)
		perror ("Proxy ERROR: Socket creation failed");

	memset(&servaddr, 0, sizeof(servaddr));
    //Init server
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(server_ip); 
	servaddr.sin_port = htons(server_port);
  
  
    // Bind the socket to the server address
    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("SERVER: Error while binding");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd, 10) < 0) {
        perror("SERVER: Error while listening");
        exit(EXIT_FAILURE);
    }

	//INit cache
  	memset(Cached_Entries, 0, MAX_NUM_CACHE * sizeof(struct cache_));

   	printf("\nProxy server is active and accepting connections.\n");
	
	while(1)
	{
		new_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &socklen);
		if (new_fd < 0) {
			perror("Server Error: Connection accept failed");
			continue; 
        }
		//Use thread to handle new connections
		if (pthread_create(&thread, NULL, (void *)start_proxy, (void*)(intptr_t)new_fd) != 0) {
			perror("Thread creation error");
            close(new_fd);
		}
    }
	//Do not end parent
}
