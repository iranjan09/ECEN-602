#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return -1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int client_socket = create_client_socket();
    if (client_socket < 0) {
        perror("Error creating client socket");
        return -1;
    }

    if (connect_to_server(client_socket, server_ip, port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    printf("Connected to the server.\n");

    char message[BUFFER_SIZE];

    while (1) {
        printf("Enter a message (Ctrl-D to exit): ");
        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;
        }

        if (send_data(client_socket, message) < 0) {
            perror("Error sending data to server");
            break;
        }

        char response[BUFFER_SIZE];
        int bytes_received = receive_data(client_socket, response, sizeof(response) - 1);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Server disconnected.\n");
            } else {
                perror("Error reading from server");
            }
            break;
        }

        response[bytes_received] = '\0';
        printf("Server's response: %s", response);
    }

    close(client_socket);
    return 0;
}
