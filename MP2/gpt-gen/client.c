#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_MESSAGE_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    int client_socket, port;
    struct sockaddr_in server_addr;
    char message[MAX_MESSAGE_SIZE];

    port = atoi(argv[2]);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        error("Error opening socket");
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        error("Invalid address");
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("Connection failed");
    }

    printf("Connected to the server. Enter your username: ");
    char username[50];
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';  // Remove the newline character

    // Send JOIN message to the server
    char join_message[MAX_MESSAGE_SIZE];
    snprintf(join_message, sizeof(join_message), "JOIN %s\n", username);
    send(client_socket, join_message, strlen(join_message), 0);

    while (1) {
        printf("Enter your message (or 'quit' to exit): ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';  // Remove the newline character

        if (strcmp(message, "quit") == 0) {
            // User wants to quit
            break;
        }

        // Send the message to the server
        char send_message[MAX_MESSAGE_SIZE];
        snprintf(send_message, sizeof(send_message), "SEND %s\n", message);
        send(client_socket, send_message, strlen(send_message), 0);
    }

    // Close the client socket
    close(client_socket);
    return 0;
}
