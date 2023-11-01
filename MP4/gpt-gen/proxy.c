#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define CACHE_SIZE 10

typedef struct {
    char url[256];
    char content[BUFFER_SIZE];
} CacheEntry;

CacheEntry cache[CACHE_SIZE];
int cacheIndex = 0;

int findInCache(const char *url) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (strcmp(cache[i].url, url) == 0) {
            return i;
        }
    }
    return -1;
}

void addToCache(const char *url, const char *content) {
    if (cacheIndex >= CACHE_SIZE) {
        // Implement LRU replacement here
        // You need to remove the least recently used entry
    }
    strcpy(cache[cacheIndex].url, url);
    strcpy(cache[cacheIndex].content, content);
    cacheIndex++;
}

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesRead;

    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead < 0) {
        perror("Error reading from client");
        close(clientSocket);
        return;
    }

    // Extract the URL from the client request
    // You'll need to parse the HTTP request to find the URL

    int cacheIndex = findInCache(url);

    if (cacheIndex != -1) {
        // Serve the response from the cache
        send(clientSocket, cache[cacheIndex].content, strlen(cache[cacheIndex].content), 0);
    } else {
        // Forward the request to the intended destination
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            perror("Error creating server socket");
            close(clientSocket);
            return;
        }

        // Connect to the target server (you'll need to resolve the host)
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(80);
        serverAddr.sin_addr.s_addr = inet_addr("target_server_ip");

        if (connect(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Error connecting to the target server");
            close(clientSocket);
            close(serverSocket);
            return;
        }

        // Forward the request to the target server
        send(serverSocket, buffer, bytesRead, 0);

        // Receive the response from the target server
        bytesRead = recv(serverSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead < 0) {
            perror("Error reading from server");
        } else {
            // Send the response to the client
            send(clientSocket, buffer, bytesRead, 0);
            
            // Cache the response
            addToCache(url, buffer);
        }

        close(serverSocket);
    }

    close(clientSocket);
}

int main(int argc, char *argv[]) {
    // Create a socket for the proxy server
    int proxySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxySocket < 0) {
        perror("Error creating proxy socket");
        return 1;
    }

    struct sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_port = htons(8080);  // Change to your desired port
    proxyAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the proxy socket to the specified port
    if (bind(proxySocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) < 0) {
        perror("Error binding proxy socket");
        return 1;
    }

    // Listen for incoming connections
    listen(proxySocket, 10);

    printf("Proxy server is running on port 8080\n");

    while (1) {
        int clientSocket;
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        // Accept incoming client connections
        clientSocket = accept(proxySocket, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSocket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        // Handle the client request
        handleClient(clientSocket);
    }

    close(proxySocket);

    return 0;
}
