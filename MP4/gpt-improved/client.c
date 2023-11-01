#include "include.h"

// Function to extract the filename from a path
char *get_filename(const char *path) {
    const char *separator = "/";
    const char *last_slash = strrchr(path, separator[0]);
    
    if (last_slash != NULL) {
        return strdup(last_slash + 1);
    } else {
        return strdup(path);
    }
}

// Function to establish a connection to the proxy server
int connect_to_proxy(const char *proxyIP, uint16_t proxyPort) {
    struct sockaddr_in proxyServer;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("Client ERROR: Socket creation failed:");
        exit(EXIT_FAILURE);
    }

    memset(&proxyServer, 0, sizeof(struct sockaddr_in));
    proxyServer.sin_family = AF_INET;
    proxyServer.sin_port = htons(proxyPort);
    int ret = inet_pton(AF_INET, proxyIP, &(proxyServer.sin_addr));

    if (ret < 0) {
        perror("Client ERROR: IP conversion failed:");
        exit(EXIT_FAILURE);
    }

    ret = connect(sockfd, (struct sockaddr *)&proxyServer, sizeof(proxyServer));

    if (ret < 0) {
        perror("Client ERROR: Socket connection failed:");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Function to send an HTTP GET request to the proxy server
int send_request(int sockfd, const char *request) {
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("Client Error: Sending Failed:");
        exit(EXIT_FAILURE);
    }

    return 0;
}

// Function to receive a response from the proxy server
int receive_response(int sockfd, char *buffer, size_t buffer_size) {
    int bytes_received = recv(sockfd, buffer, buffer_size, 0);

    if (bytes_received <= 0) {
        perror("Client Error: Unable to receive from proxy:");
    }

    return bytes_received;
}

// Function to save the response to a file
int save_response_to_file(const char *filename, const char *response, int response_length) {
    FILE *filePointer = fopen(filename, "w");

    if (filePointer == NULL) {
        perror("Client Error: Failed to open file for writing:");
        return 1;
    }

    // Find the start of the response body
    char *headerEnd = strstr(response, "\r\n\r\n");
    if (headerEnd != NULL) {
        fwrite(headerEnd + 4, 1, response_length - (headerEnd - response + 4), filePointer);
    }

    fclose(filePointer);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: ./client <Proxy IP> <Port> <URL>\n");
        return 1;
    }

    const char *proxyIP = argv[1];
    uint16_t proxyPort = atoi(argv[2]);
    const char *url = argv[3];
    char request[200];
    char buffer[BUFFER_SIZE];

    // Parse URL, create GET request, and extract filename
    char hostname[256] = {0};
    char path[512] = {0};
    
    if (sscanf(url, "http://%255[^/]/%511s", hostname, path) < 2) {
        sscanf(url, "http://%255[^/]", hostname);
    }

    int requestSize = snprintf(request, sizeof(request), "GET /%s HTTP/1.0\r\n", path);

    if (requestSize < 0 || requestSize >= sizeof(request)) {
        printf("Client ERROR: Request buffer overflow\n");
        return 1;
    }

    printf("Proxy IP: %s\nProxy Port: %d\nURL: %s\n", proxyIP, proxyPort, url);
    printf("Hostname: %s\nPath: %s\n", hostname, path);
    printf("GET Request:\n%s", request);

    int sockfd = connect_to_proxy(proxyIP, proxyPort);
    send_request(sockfd, request);
    int responseLength = receive_response(sockfd, buffer, BUFFER_SIZE);

    if (responseLength > 0) {
        char *filename = get_filename(path);
        int result = save_response_to_file(filename, buffer, responseLength);

        if (result == 0) {
            printf("Received response from server\n");
        }

        free(filename);
    }

    close(sockfd);
    return 0;
}
