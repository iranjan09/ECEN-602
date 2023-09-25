#include "common.h"

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE + 1];
    int n;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE + 1);
        n = receive_data(client_sock, buffer, BUFFER_SIZE);
        if (n <= 0) {
            if (n == 0) {
                printf("SERVER: Client disconnected.\n");
            } else {
                perror("SERVER: Error reading from socket");
            }
            close(client_sock);
            exit(0);
        }
        
        printf("SERVER: Receiving Data\n");
        printf("SERVER: Received: %s\n", buffer);
        printf("SERVER: Echoing: %s\n", buffer);

        send_data(client_sock, buffer); // echo back to the client
    }
}

int main(int argc, char *argv[]) {

    /*Input: Server Port number; data_type: int*/

    int sockfd, client_sock;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int portno = atoi(argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("SERVER: Error opening socket");
        exit(1);
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);


    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("SERVER: Error on binding");
        exit(1);
    }

    printf("SERVER: Server Initialized, waiting for clients\nSERVER MAX CAPACITY: %d clients\n", MAX_CLIENT_SIZE);
    
    // Current number of max clients is 100
    listen(sockfd, MAX_CLIENT_SIZE);
    clilen = sizeof(cli_addr);

    printf("SERVER: got address info, listening\n");

    while (1) {
        client_sock = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (client_sock < 0) {
            perror("SERVER: Error on accepting connection");
            continue;
        }

        printf("SERVER: Connection accepted from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        pid_t pid = fork();
        if (pid == 0) { //  pid =0 means we are in the child process
            close(sockfd); // child doesn't need the listening socket
            handle_client(client_sock);
        } else if (pid > 0){
            printf("SERVER: Child process created with PID: %d\n", pid);
        } else {
            printf("SERVER: Failure while forking");
        } 
    }

    close(sockfd);
    return 0;
}
