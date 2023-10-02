#include "common.h"

int get_server_address_type(const char* server_address) {
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;

    // Try to parse as IPv4 address
    if (inet_pton(AF_INET, server_address, &ipv4_addr) == 1) {
        return AF_INET; // IPv4 address
    }
    // Try to parse as IPv6 address
    else if (inet_pton(AF_INET6, server_address, &ipv6_addr) == 1) {
        return AF_INET6; 
    } else {
        return -1; 
    }
}

int handle_name(char *name, int client_count, int max_client, struct user_data *clients) {
    int i;
    if (client_count == max_client) {
        return -1;
    }
    for (i = 0; i < client_count; i++) {
        if (strcmp(name, clients[i].user_name) == 0) {
            return -2;
        }
    }
    return 0;
}

void send_ack_message(int new_fd, int client_count, struct user_data *clients, const char *username) {
    int i;
    sbcp_message_t ack_message;
    sbcp_header_t ack_header;
    sbcp_attribute_t ack_attribute;

    ack_header.uiVrsn = 3;
    ack_header.uiType = 7;

    ack_attribute.uiType = 4;

    char msg[512];
    char client_num[76];
    sprintf(client_num, "SERVER: There are %d users, The other users are ", client_count);
    strcpy(msg, client_num);
    strcat(msg, " ");
    for (i = 0; i < client_count - 1; i++) {
        if (clients[i].user_name != username) {
            strcat(msg, clients[i].user_name);
            if (i < (client_count - 2)) {
                strcat(msg, ",");
            }
        }
    }
    ack_attribute.uiLength = strlen(msg) + 1;
    strcpy(ack_attribute.acPayload, msg);
    ack_message.sMsgHeader = ack_header;
    ack_message.sMsgAttribute = ack_attribute;
    printf("ACK message sent to %s\n", username);

    send(new_fd, (void *)&ack_message, sizeof(ack_message), 0);
}

void send_nack_message(int new_fd, int client_count, struct user_data *clients, int code) {
    sbcp_message_t nack_message;
    sbcp_header_t nack_header;
    sbcp_attribute_t nack_attribute;

    char msg[130];

    nack_header.uiVrsn = 3;
    nack_header.uiType = 5;

    nack_attribute.uiType = 1;

    if (code == -1) {
        strcpy(msg, "Chatroom full, try later\n");
    } else if (code == -2) {
        strcpy(msg, "Username already taken, try another one\n");
    }

    nack_attribute.uiLength = strlen(msg);
    strcpy(nack_attribute.acPayload, msg);

    nack_message.sMsgHeader = nack_header;
    nack_message.sMsgAttribute = nack_attribute;

    send(new_fd, (void *)&nack_message, sizeof(nack_message), 0);

    close(new_fd);
}

int server_listen(int new_fd, int *client_count, int max_client, struct user_data *clients) {
    sbcp_message_t join_message;
    sbcp_attribute_t join_message_attr;
    recv(new_fd, (struct message *)&join_message, sizeof(join_message), 0);
    join_message_attr = join_message.sMsgAttribute;
    char name[16];
    int ret = 0;
    strcpy(name, join_message_attr.acPayload);
    ret = handle_name(name, *client_count, max_client, clients);
    if (ret < 0) {
        printf("SERVER: Sending NAK.\n");
        send_nack_message(new_fd, *client_count, clients, ret);
        return -1;
    } else {
        strcpy(clients[*client_count].user_name, name);
        clients[*client_count].socket_fd = new_fd;
        clients[*client_count].user_number = *client_count;
        *client_count = (*client_count) + 1;
        send_ack_message(new_fd, *client_count, clients, name);
    }
    return 0;
}

void remove_client(struct user_data *clients, int socket_fd, int client_count) {
    int index = 0;
    for (index = 0; index < client_count; index++) {
        if (clients[index].socket_fd == socket_fd) {
            break;
        }
    }
    while (index + 1 < client_count) {
        clients[index] = clients[index + 1];
        index++;
    }
}

sbcp_message_t *get_off_message(char *new_user_name) {
    sbcp_message_t *offline_message = malloc(sizeof(sbcp_message_t));
    (*offline_message).sMsgHeader.uiType = 3;
    (*offline_message).sMsgHeader.uiVrsn = 3;
    (*offline_message).sMsgAttribute.uiType = 4;
    strcat((*offline_message).sMsgAttribute.acPayload, new_user_name);
    strcat((*offline_message).sMsgAttribute.acPayload, " is now OFFLINE.\n");
    return offline_message;
}

void server_send(int listening_fd, int socket_fd, struct user_data *clients, int fdmax, fd_set *master, int *client_count) {
    sbcp_message_t message_from_client;
    sbcp_message_t message_to_client;

    int index, j;
    int recv_bytes = recv(socket_fd, (sbcp_message_t *)&message_from_client, sizeof(sbcp_message_t), 0);
    if (recv_bytes <= 0) {
        if (recv_bytes == 0) {
            sbcp_message_t *offline_message;

            for (index = 0; index < (*client_count); index++) {
                if (clients[index].socket_fd == socket_fd) {
                    printf("SERVER: Connection closed by %s\n", clients[index].user_name);
                    offline_message = get_off_message(clients[index].user_name);
                    break;
                }
            }
            for (j = 0; j <= fdmax; j++) {
                if (FD_ISSET(j, master)) {
                    if (j != listening_fd && j != socket_fd) {
                        if (send(j, (void *)offline_message, sizeof(sbcp_message_t), 0) == -1) {
                            perror("Sending Message Error: ");
                            exit(1);
                        }
                    }
                }
            }
        } else {
            perror("Error receiving message");
            exit(1);
        }
        close(socket_fd);
        FD_CLR(socket_fd, master);
        remove_client(clients, socket_fd, *client_count);
        (*client_count) = (*client_count) - 1;
    } else {
        // Messages

        message_to_client.sMsgHeader.uiType = 3;
        message_to_client.sMsgHeader.uiVrsn = 3;
        message_to_client.sMsgAttribute.uiType = 4;

        for (index = 0; index <= (*client_count); index++) {
            if (clients[index].socket_fd == socket_fd) {
                strcpy(message_to_client.sMsgAttribute.acPayload, clients[index].user_name);
                if (message_from_client.sMsgHeader.uiType == IDLE) {
                    strcat(message_to_client.sMsgAttribute.acPayload, " is IDLE");
                } else {
                    strcat(message_to_client.sMsgAttribute.acPayload, ": ");
                    strcat(message_to_client.sMsgAttribute.acPayload, message_from_client.sMsgAttribute.acPayload);
                }
                strcat(message_to_client.sMsgAttribute.acPayload, "\n");
                break;
            }
        }
        for (index = 0; index <= fdmax; index++) {
            if (FD_ISSET(index, master)) {
                if (index != listening_fd && index != socket_fd) {
                    if (send(index, (void *)&message_to_client, sizeof(sbcp_message_t), 0) == -1) {
                        perror("Sending Message Error: ");
                        exit(1);
                    }
                }
            }
        }
    }
}

sbcp_message_t *get_online_message(char *new_user_name) {
    sbcp_message_t *online_msg = malloc(sizeof(sbcp_message_t));
    (*online_msg).sMsgHeader.uiType = 3;
    (*online_msg).sMsgHeader.uiVrsn = 3;
    (*online_msg).sMsgAttribute.uiType = 4;
    strcat((*online_msg).sMsgAttribute.acPayload, new_user_name);
    strcat((*online_msg).sMsgAttribute.acPayload, " ");
    strcat((*online_msg).sMsgAttribute.acPayload, "is now ONLINE.\n");
    return online_msg;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "SERVER: Invlid input\n Correct Input: %s <server_ip> <server_port> <max_clients>\n", argv[0]);
        exit(1);
    }

    int opt = 1;

    // Parse command line arguments
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int max_client = atoi(argv[3]);
    int server_address_type;
    int new_fd;
    struct user_data *clients;

    int k;

    server_address_type = get_server_address_type(server_ip);

    if (server_address_type == -1) {
        printf("SERVER: Invalid server address: %s\n", server_ip);
        return 1;
    }

    int listen_fd;
    if (server_address_type == AF_INET) {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    } else if (server_address_type == AF_INET6) {
        listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        fprintf(stderr, "SERVER: Unsupported address type\n");
        return 1;
    }

    if (listen_fd == -1) {
        perror("SERVER: Error opening socket\n");
        exit(1);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("SERVER: Error on setsockopt\n");
        exit(1);
    }

    struct sockaddr_storage server_addr;
    struct sockaddr_in *client_addresses;
    memset(&server_addr, 0, sizeof(server_addr));

    if (server_address_type == AF_INET) {
        struct sockaddr_in *server_addr4 = (struct sockaddr_in *)&server_addr;
        server_addr4->sin_family = AF_INET;
        server_addr4->sin_addr.s_addr = inet_addr(server_ip);
        server_addr4->sin_port = htons(server_port);
    } else if (server_address_type == AF_INET6) {
        struct sockaddr_in6 *server_addr6 = (struct sockaddr_in6 *)&server_addr;
        server_addr6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, server_ip, &(server_addr6->sin6_addr));
        server_addr6->sin6_port = htons(server_port);
    }

    client_addresses = (struct sockaddr_in *)malloc(max_client * sizeof(struct sockaddr_in));
    clients = (struct user_data *)malloc(max_client * sizeof(struct user_data));
    int client_count = 0;

    // Bind the socket to the server address
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("SERVER: Error while binding");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(listen_fd, max_client) == -1) {
        perror("SERVER: Error while listening");
        exit(1);
    }

    // Initialize the set of active sockets
    fd_set read_fds;
    fd_set master;
    FD_ZERO(&read_fds);
    FD_ZERO(&master);
    FD_SET(listen_fd, &master);
    int fdmax = listen_fd;

    int fcur = 0;

    printf("SERVER: Server intialized\n");
    printf("SERVER: IP Address Type: %s\n", (server_address_type == AF_INET) ? "IPv4" : "IPv6");
    printf("SERVER: Port number is %d\n", server_port);
    printf("SERVER: Maximum Client Capacity is %d\n", max_client);

    while (1) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Select Error: ");
            exit(1);
        }
        for (fcur = 0; fcur <= fdmax; fcur++) {
            if (FD_ISSET(fcur, &read_fds)) {
                if (fcur == listen_fd) {
                    socklen_t len = (socklen_t)sizeof(client_addresses[client_count]);
                    int new_fd = accept(listen_fd, (struct sockaddr *)&client_addresses[client_count], &len);
                    if (new_fd < 0) {
                        perror("Accept error: ");
                        exit(1);
                    }
                    int nak_flag = server_listen(new_fd, &client_count, max_client, clients);
                    if (nak_flag != -1) {
                        if (new_fd > fdmax) {
                            fdmax = new_fd;
                        }
                        FD_SET(new_fd, &master);
                        sbcp_message_t *join_message = get_online_message(clients[client_count - 1].user_name);
                        for (k = 0; k <= fdmax; k++) {
                            if (FD_ISSET(k, &master)) {
                                if (k != new_fd && k != listen_fd) {
                                    if (send(k, (void *)join_message, sizeof(sbcp_message_t), 0) == -1) {
                                        perror("Sending Message Error: ");
                                        exit(1);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    server_send(listen_fd, fcur, clients, fdmax, &master, &client_count);
                }
            }
        }
    }

    close(listen_fd);
}
