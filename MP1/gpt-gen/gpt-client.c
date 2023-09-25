#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    while (1) {
        // Read a line of text from the user
        printf("Enter a message (Ctrl-D to exit): ");
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        // Send the message to the server
        send(client_socket, message, strlen(message), 0);

        // Receive and print the echoed message from the server
        recv(client_socket, message, BUFFER_SIZE, 0);
        printf("Server's response: %s", message);
    }

    // Close the client socket
    close(client_socket);

    return 0;
}
