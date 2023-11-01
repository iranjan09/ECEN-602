#include "include.h"

// Function to display all cache entries
void Dump_Cache() {
    if (cache_count == 0) {
        printf("Cache is currently empty.\n");
    } else {
        printf("Cache Entries:\n");
        for (int index = 0; index < cache_count; index++) {
            PrintCacheEntry(&Cached_Entries[index]);
        }
    }
}

// Function to print a single cache entry
void PrintCacheEntry(struct cache_ *entry) {
    printf("Cache Entry:\n");
    printf("URL: %s\n", entry->URL);
    printf("Access Date: %s\n", entry->Access_Date);
    printf("Expires: %s\n", strcmp(entry->Expires, "") != 0 ? entry->Expires : "N/A");
    printf("Last Modified: %s\n", strcmp(entry->Last_Modified, "") != 0 ? entry->Last_Modified : "N/A");
}

// Function to update or add a cache entry
void UpdateCacheEntry(char *target, char *info, int newEntry, int entry) {
    if (newEntry == 1) {
        if (cache_count == MAX_NUM_CACHE) {
            ClearCacheEntry(&Cached_Entries[0]);
            ShiftCacheEntries();
        } else {
            InitializeCacheEntry(&Cached_Entries[cache_count]);
            cache_count++;
        }
    } else {
        // Update the existing entry
        UpdateExistingCacheEntry(target, entry);
    }
}

// Function to clear a cache entry
void ClearCacheEntry(struct cache_ *entry) {
    memset(entry, 0, sizeof(struct cache_));
}

// Function to initialize a new cache entry
void InitializeCacheEntry(struct cache_ *entry) {
    ClearCacheEntry(entry);
    entry->content = (char *)malloc(1); // Initialize with a null character
}

// Function to update an existing cache entry
void UpdateExistingCacheEntry(char *target, int entry) {
    // Shift entries to make room for the updated entry
    for (int entries = entry; entries < cache_count; entries++) {
        if (entries == cache_count - 1) {
            break;
        }
        Cached_Entries[entries] = Cached_Entries[entries + 1];
    }

    Cached_Entries[cache_count - 1] = Cached_Entries[entry]; // Put the entry at the end
    UpdateAccessTime(&Cached_Entries[cache_count - 1]);
}

// Function to shift cache entries
void ShiftCacheEntries() {
    for (int entries = 0; entries < MAX_NUM_CACHE; entries++) {
        if (entries + 1 != MAX_NUM_CACHE) {
            Cached_Entries[entries] = Cached_Entries[entries + 1];
        } else {
            ClearCacheEntry(&Cached_Entries[entries]);
        }
    }
}

// Function to update the access time of a cache entry
void UpdateAccessTime(struct cache_ *entry) {
    time_t now = time(NULL);
    struct tm current_time = *gmtime(&now);
    const char *date_format = "%a, %d %b %Y %H:%M:%S GMT";
    strftime(entry->Access_Date, sizeof(entry->Access_Date), date_format, &current_time);
}

// Function to check if a cache entry is fresh
int IsCacheEntryFresh(int entry) {
    time_t currentTime = time(NULL);
    struct tm currentGMTTime = *gmtime(&currentTime);
    struct tm expiresTime;

    if (strcmp(Cached_Entries[entry].Expires, "") != 0) {
        if (strptime(Cached_Entries[entry].Expires, "%a, %d %b %Y %H:%M:%S %Z", &expiresTime) != NULL) {
            time_t expires = mktime(&expiresTime);
            time_t now = mktime(&currentGMTTime);

            if (expires != -1 && now != -1) {
                if (difftime(now, expires) < 0) {
                    return 1;
                }
            }
        }
    }

    return -1;
}

// Function to connect to a remote host
int ConnectToHost(const char *hostname) {
    int websocket = -1;
    int s = -1;
    struct addrinfo hints, *result, *rp;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((s = getaddrinfo(hostname, "http", &hints, &result)) != 0) {
        fprintf(stderr, "Server: getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        websocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (websocket >= 0 && (connect(websocket, rp->ai_addr, rp->ai_addrlen) >= 0)) {
            break;
        }
    }

    if (rp == NULL) {
        websocket = -1;
    }

    freeaddrinfo(result);

    return websocket;
}

// Function to parse the response from a remote page
int ParseMessageBody(int sockfd, char *messageBuffer) {
    int totalBytesRead = 0;
    int bytesRead = 1;
    char dataBuffer[MAX_MESSAGE_LENGTH] = {0};

    while (bytesRead > 0) {
        memset(dataBuffer, 0, sizeof(dataBuffer));
        bytesRead = read(sockfd, dataBuffer, MAX_MESSAGE_LENGTH);

        if (bytesRead <= 0) {
            break;
        }

        strcat(messageBuffer, dataBuffer);
        totalBytesRead += bytesRead;

        if (dataBuffer[bytesRead - 1] == EOF) {
            strncpy(messageBuffer, messageBuffer, (strlen(messageBuffer) - 1));
            totalBytesRead--;
            break;
        }
    }

    return totalBytesRead;
}

/// Function to handle a new proxy connection
void StartProxy(int sockfd) {
    char *recv_buf;
    char reqtype[8] = {0}, target[256] = {0}, httpVersion[16] = {0};
    int cachedEntry, isNotExpired = -1;
    char *dataToSend = NULL, *response = NULL;
    char host[64], path[256], urlSegment[256] = {0};
    int websocket, wport = 80;
    char msg_gen[256] = {0};
    int bytesRead = 0;
    char client_req[MAX_MESSAGE_LENGTH] = {0};

    recv_buf = (char *)calloc(MAX_MESSAGE_LENGTH, sizeof(char));

    if (read(sockfd, recv_buf, MAX_MESSAGE_LENGTH) < 0) {
        perror("Server Error: Unable to read message from client");
        free(recv_buf);
        return;
    }

    printf("Server: Received request for page:\n%s", recv_buf);

    sscanf(recv_buf, "%s %s %s", reqtype, target, httpVersion);

    cachedEntry = Cache_FindElement(target);

    if (cachedEntry != -1)
        isNotExpired = IsCacheEntryFresh(cachedEntry);

    if (cachedEntry != -1 && isNotExpired) {
        HandleCachedRequest(target, &dataToSend);
    } else {
        memset(path, 0, 256);
        memset(host, 0, 64);
        memcpy(&urlSegment[0], &target[0], 256);
        targetParser(urlSegment, host, &wport, path);

        if ((websocket = ConnectToHost(host)) == -1) {
            perror("Server ERROR: Proxy cannot connect to host");
            free(recv_buf);
            return;
        }

        printf("Server: Connected successfully to remote server %d\n", websocket);

        if (cachedEntry != -1) {
            HandleCachedEntryExpired(cachedEntry, reqtype, path, httpVersion, host, &dataToSend, &response);
        } else {
            HandleUncachedRequest(reqtype, path, httpVersion, host, websocket, &dataToSend, &response);
        }
    }

    free(recv_buf);
    Dump_Cache();
    write(sockfd, dataToSend, strlen(dataToSend) + 1);
}

// Function to handle a cached request
void HandleCachedRequest(char* target, char** dataToSend) {
    printf("Server: Client request for URL: %s is present in the cache and is not expired\n");
    UpdateCacheEntry(target, NULL, 0, cachedEntry);
    *dataToSend = (char *)calloc(strlen(Cached_Entries[cachedEntry].content) + 1, sizeof(char));
    memcpy(*dataToSend, Cached_Entries[cachedEntry].content, strlen(Cached_Entries[cachedEntry].content));
}

// Function to handle an expired cached entry
void HandleCachedEntryExpired(int cachedEntry, char* reqtype, char* path, char* httpVersion, char* host, char** dataToSend, char** response) {
    printf("Server: URL %s in cache has expired.\n");
    char ifModifiedSince[60];

    // ... Rest of the code for handling expired cached entry ...

    if (strstr(response, "304 Not Modified") != NULL) {
        printf("Received '304 Not Modified' response. Sending cached file instead.\n");
        memcpy(*dataToSend, Cached_Entries[cachedEntry].content, strlen(Cached_Entries[cachedEntry].content));
        UpdateCacheEntry(target, NULL, 0, cachedEntry);
    } else {
        printf("Server: File stored in cache was modified, update it\n");
        memcpy(*dataToSend, response, strlen(response));
        UpdateCacheEntry(target, NULL, 0, cachedEntry);
        Cached_Entries[--cache_count] = Clear_Entry;
        UpdateCacheEntry(target, response, 1, 0);
    }
}

// Function to handle an uncached request
void HandleUncachedRequest(char* reqtype, char* path, char* httpVersion, char* host, int websocket, char** dataToSend, char** response) {
    printf("Server: Client requested URL is not present in cache. Fetch it.\n");
    char client_req[MAX_MESSAGE_LENGTH] = {0};
    memset(client_req, 0, MAX_MESSAGE_LENGTH);
    snprintf(client_req, MAX_MESSAGE_LENGTH, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\n\r\n", reqtype, path, httpVersion, host);
    printf("Server: Generated Request from Client: \n%s", client_req);
    write(websocket, client_req, MAX_MESSAGE_LENGTH);
    *response = (char *)malloc(100000);
    int bytesRead = ParseMessageBody(websocket, *response);
    *dataToSend = (char *)malloc(strlen(*response));
    memcpy(*dataToSend, *response, strlen(*response));
    UpdateCacheEntry(target, *response, 1, 0);
}


#include "include.h"

// Initialize the proxy server
int initializeProxy(const char *serverIP, int serverPort, int *listen_fd) {
    struct sockaddr_in servaddr;
    socklen_t socklen;

    *listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (*listen_fd < 0) {
        perror("Proxy ERROR: Socket creation failed");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serverIP);
    servaddr.sin_port = htons(serverPort);

    if (bind(*listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("SERVER: Error while binding");
        return 1;
    }

    if (listen(*listen_fd, 10) < 0) {
        perror("SERVER: Error while listening");
        return 1;
    }

    return 0;
}

// Handle a new client connection using a thread
void handleClientConnection(int new_fd) {
    pthread_t thread;

    if (pthread_create(&thread, NULL, (void *)start_proxy, (void *)(intptr_t)new_fd) != 0) {
        perror("Thread creation error");
        close(new_fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Proxy ERROR: Please provide Proxy Server IP address and port number\n");
        printf("Usage: ./proxy <Proxy IP> <Port>");
        exit(EXIT_FAILURE);
    }

    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2);
    int listen_fd;

    if (initializeProxy(serverIP, serverPort, &listen_fd) != 0) {
        exit(EXIT_FAILURE);
    }

    printf("\nProxy server is active and accepting connections.\n");

    while (1) {
        int new_fd;
        struct sockaddr_storage client_addr;
        socklen_t socklen = sizeof(client_addr);

        new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &socklen);

        if (new_fd < 0) {
            perror("Server Error: Connection accept failed");
            continue;
        }

        handleClientConnection(new_fd);
    }
    // Do not end the parent
}
