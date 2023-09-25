#include "common.h"

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        n = receive_data(client_sock, buffer, sizeof(buffer) - 1);

        if (n <= 0) {
            if (n == 0) {
                printf("Client disconnected.\n");
            } else {
                perror("Error reading from socket");
            }
            close(client_sock);
            exit(0);
        }

        printf("Received: %s", buffer);

        if (send_data(client_sock, buffer) < 0) {
            perror("Error sending data to client");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);

    int server_socket = create_server_socket(port);
    if (server_socket < 0) {
        perror("Error creating server socket");
        return -1;
    }

    printf("Server is listening on port %d...\n", port);

    while (1) {
        int client_socket = accept_client_connection(server_socket);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        printf("Connection from %s:%d\n", get_client_ip(client_socket), get_client_port(client_socket));

        pid_t pid = fork();
        if (pid == 0) { // Child process
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        } else if (pid < 0) {
            perror("Error forking process");
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}
