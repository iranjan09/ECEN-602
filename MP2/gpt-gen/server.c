#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_SIZE 1024

struct Client {
    int socket;
    char username[50];
    int active; // Flag to track active clients
};

struct Client clients[MAX_CLIENTS];
int num_clients = 0;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void broadcast(const char *message, int sender_socket) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket != sender_socket) {
            // Send the message to all clients except the sender
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int server_socket, port;
    fd_set readfds;
    char message[MAX_MESSAGE_SIZE];

    port = atoi(argv[1]);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        error("Error opening socket");
    }

    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("Error binding");
    }

    listen(server_socket, 5);
    printf("Server listening on port %d...\n", port);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        int max_fd = server_socket;

        for (int i = 0; i < num_clients; i++) {
            int client_socket = clients[i].socket;
            FD_SET(client_socket, &readfds);

            if (client_socket > max_fd) {
                max_fd = client_socket;
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            error("Error in select");
        }

        if (FD_ISSET(server_socket, &readfds)) {
            int client_socket = accept(server_socket, NULL, NULL);
            if (client_socket < 0) {
                error("Error accepting connection");
            }

            // Handle a new client
            if (num_clients >= MAX_CLIENTS) {
                send(client_socket, "NACK Server full. Try again later.\n", 34, 0);
                close(client_socket);
            } else {
                clients[num_clients].socket = client_socket;
                clients[num_clients].active = 1;
                num_clients++;

                printf("Client connected\n");
            }
        }

        for (int i = 0; i < num_clients; i++) {
            int client_socket = clients[i].socket;

            if (FD_ISSET(client_socket, &readfds)) {
                int bytes_received = recv(client_socket, message, sizeof(message), 0);
                if (bytes_received <= 0) {
                    // Client disconnected
                    close(client_socket);
                    printf("Client disconnected\n");

                    // Notify other clients
                    sprintf(message, "FWD Server: %s has left the chat.\n", clients[i].username);
                    broadcast(message, client_socket);

                    // Remove the client from the list
                    for (int j = i; j < num_clients - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    num_clients--;
                } else {
                    message[bytes_received] = '\0';

                    // Handle client messages here
                    // You should implement message parsing and broadcasting logic
                    // Example: broadcast(message, client_socket);
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
