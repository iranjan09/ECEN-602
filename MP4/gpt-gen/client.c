#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void sendHttpRequest(int proxySocket, const char* url) {
    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "GET %s HTTP/1.0\r\n\r\n", url);

    send(proxySocket, request, strlen(request), 0);
}

void receiveHttpResponse(int proxySocket) {
    char response[BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = recv(proxySocket, response, BUFFER_SIZE, 0)) > 0) {
        // Print or save the received data
        write(1, response, bytesRead);  // Write to stdout
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s proxy_server_address proxy_server_port url\n", argv[0]);
        return 1;
    }

    const char *proxyServerAddress = argv[1];
    const int proxyServerPort = atoi(argv[2);
    const char *url = argv[3];

    int proxySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxySocket < 0) {
        perror("Error creating proxy socket");
        return 1;
    }

    struct sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_port = htons(proxyServerPort);
    proxyAddr.sin_addr.s_addr = inet_addr(proxyServerAddress);

    if (connect(proxySocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) < 0) {
        perror("Error connecting to the proxy server");
        close(proxySocket);
        return 1;
    }

    sendHttpRequest(proxySocket, url);
    receiveHttpResponse(proxySocket);

    close(proxySocket);

    return 0;
}
