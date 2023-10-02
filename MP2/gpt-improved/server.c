#include "common.h"

void set_server_address(struct sockaddr_storage *server_address, const char *ip, int port, int ipv6) {
    memset(server_address, 0, sizeof(*server_address));

    if (ipv6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)server_address;
        addr6->sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, ip, &(addr6->sin6_addr)) <= 0) {
            perror("inet_pton error");
            exit(EXIT_FAILURE);
        }
        addr6->sin6_port = htons(port);
    } else {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)server_address;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = inet_addr(ip);
        addr4->sin_port = htons(port);
    }
}

void bind_server(int socket_fd, struct sockaddr_storage server_address) {
    if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) != 0) {
        perror("Socket Bind Failed");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket is bound successfully...\n");
    }
}

void start_listening(int socket_fd) {
    if (listen(socket_fd, QUEUE_SIZE) != 0) {
        perror("Listening Failed");
        exit(EXIT_FAILURE);
    } else {
        printf("Server Listening Started...\n");
    }
}

int accept_connection(struct sockaddr_storage *client_address, socklen_t *client_addr_len, int socket_fd) {
    int new_socket_fd = accept(socket_fd, (struct sockaddr *)client_address, client_addr_len);
    if (new_socket_fd < 0) {
        perror("Accept connection error: ");
        exit(EXIT_FAILURE);
    }
    return new_socket_fd;
}

int check_name(const char *name, int client_count, int max_client, struct user_data *clients) {
    if (client_count == max_client) {
        return -1; // Max clients reached
    }
    for (int i = 0; i < client_count; i++) {
        if (strcmp(name, clients[i].user_name) == 0) {
            return -2; // Duplicate username
        }
    }
    return 0; // Name is valid
}

void send_ack_message(int new_client_fd, int client_count, struct user_data *clients) {
    char ack_message[512];
    snprintf(ack_message, sizeof(ack_message), "%d ", client_count);

    for (int i = 0; i < client_count - 1; i++) {
        strcat(ack_message, clients[i].user_name);
        if (i < (client_count - 2)) {
            strcat(ack_message, ",");
        }
    }

    sbcp_message_t ack_sbcp_message;
    fill_sbcp_message(&ack_sbcp_message, ACK, ack_message);

    if (send(new_client_fd, &ack_sbcp_message, sizeof(sbcp_message_t), 0) == -1) {
        perror("Sending ACK Message Error: ");
        exit(EXIT_FAILURE);
    }
}

void send_nack_message(int new_client_fd, int code) {
    const char *error_msg = (code == -1) ? "Max clients are already present. Wait till the chatroom is free" : "Duplicate username entered. Please enter a unique username";
    sbcp_message_t nack_sbcp_message;
    fill_sbcp_message(&nack_sbcp_message, NACK, error_msg);

    if (send(new_client_fd, &nack_sbcp_message, sizeof(sbcp_message_t), 0) == -1) {
        perror("Sending NACK Message Error: ");
        exit(EXIT_FAILURE);
    }

    close(new_client_fd);
}

void join_message_process(int new_client_fd, int *client_count, int max_client, struct user_data *clients) {
    sbcp_message_t join_message;
    recv(new_client_fd, &join_message, sizeof(sbcp_message_t), 0);

    sbcp_attribute_t join_message_attr = join_message.sMsgAttribute;
    char name[USERNAME_MAX_LEN];
    strcpy(name, join_message_attr.acPayload);

    int ret = check_name(name, *client_count, max_client, clients);

    if (ret < 0) {
        printf("Sending NACK.\n");
        send_nack_message(new_client_fd, ret);
    } else {
        strcpy(clients[*client_count].user_name, name);
        clients[*client_count].socket_fd = new_client_fd;
        clients[*client_count].user_number = *client_count;
        (*client_count)++;
        send_ack_message(new_client_fd, *client_count, clients);
    }
}

void remove_client(struct user_data *clients, int socket_fd, int *client_count) {
    for (int i = 0; i < *client_count; i++) {
        if (clients[i].socket_fd == socket_fd) {
            for (int j = i; j < *client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            (*client_count)--;
            break;
        }
    }
}

void broadcast_message(int listening_fd, int socket_fd, struct user_data *clients, int max_fd, fd_set *set1, int *client_count) {
    sbcp_message_t message_from_client;
    int recv_bytes = recv(socket_fd, &message_from_client, sizeof(sbcp_message_t), 0);

    if (recv_bytes <= 0) {
        if (recv_bytes == 0) {
            printf("Connection closed by socket %d", socket_fd);
            sbcp_message_t *hung_message = get_hung_message(socket_fd, clients, *client_count);

            for (int j = 0; j <= max_fd; j++) {
                if (FD_ISSET(j, set1)) {
                    if (j != listening_fd && j != socket_fd) {
                        if (send(j, hung_message, sizeof(sbcp_message_t), 0) == -1) {
                            perror("Sending Message Error: ");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        } else {
            perror("Error receiving message");
            exit(EXIT_FAILURE);
        }
        close(socket_fd);
        FD_CLR(socket_fd, set1);
        remove_client(clients, socket_fd, client_count);
    } else {
        // Process and broadcast the received message
        // ...
    }
}

sbcp_message_t *get_join_message(const char *new_user_name, int client_count, struct user_data *clients) {
    sbcp_message_t *message_to_client = malloc(sizeof(sbcp_message_t));
    char join_message[512];
    snprintf(join_message, sizeof(join_message), "%s joined the chat.\n", new_user_name);
    fill_sbcp_message(message_to_client, FWD, join_message);
    return message_to_client;
}

sbcp_message_t *get_hung_message(int socket_fd, struct user_data *clients, int client_count) {
    sbcp_message_t *message_to_client = malloc(sizeof(sbcp_message_t));
    char hung_message[512];
    char *username = NULL;

    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket_fd == socket_fd) {
            username = clients[i].user_name;
            break;
        }
    }

    if (username) {
        snprintf(hung_message, sizeof(hung_message), "%s left the chat.\n", username);
        fill_sbcp_message(message_to_client, FWD, hung_message);
    } else {
        // Handle the error gracefully
        // ...
    }

    return message_to_client;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Input Error: Please provide command line arguments in the format: IP Port Maximum_Users\n");
        exit(EXIT_FAILURE);
    }

    // Variable declaration
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int socket_fd = -1;
    struct sockaddr_storage server_address;
    struct sockaddr_in6 servaddr;
    socklen_t server_addr_len;
    int socket_itr;

    // Maximum number of connections
    int max_clients = atoi(argv[3]);
    int client_count = 0;

    // Variables for select
    fd_set read_fds, all_fds;
    int max_fd;

    // All user-related data
    struct user_data *clients;
    sbcp_message_t *join_message;

    memset(&server_address, 0, sizeof(server_address));

    if (create_socket_and_bind(&socket_fd, &server_address, &servaddr, ip, port) == -1) {
        fprintf(stderr, "Socket creation and binding failed\n");
        exit(EXIT_FAILURE);
    }

    // Initialization
    start_listening(socket_fd);

    // Allocate memory for client data and join message
    clients = (struct user_data *)malloc(max_clients * sizeof(struct user_data));
    join_message = (sbcp_message_t *)malloc(sizeof(sbcp_message_t));

    if (clients == NULL || join_message == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize sets for select
    FD_ZERO(&all_fds);
    FD_SET(socket_fd, &all_fds);
    max_fd = socket_fd;

    while (1) {
        read_fds = all_fds;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Select Error: ");
            exit(EXIT_FAILURE);
        }

        for (socket_itr = 0; socket_itr <= max_fd; socket_itr++) {
            if (FD_ISSET(socket_itr, &read_fds)) {
                if (socket_itr == socket_fd) {
                    int new_client_fd = accept_connection(&server_address, &server_addr_len, socket_fd);
                    if (new_client_fd != -1) {
                        if (client_count < max_clients) {
                            handle_new_client(new_client_fd, clients, &client_count, &max_fd, join_message, socket_fd);
                        } else {
                            // Reject the client if the maximum number of clients has been reached
                            send_reject_message(new_client_fd, "Maximum user limit reached.");
                            close(new_client_fd);
                        }
                    }
                } else {
                    handle_client_message(socket_itr, clients, &client_count, max_fd, &all_fds);
                }
            }
        }
    }

    close(socket_fd);
    free(clients);
    free(join_message);

    return 0;
}
