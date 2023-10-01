#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <max_clients>\n", argv[0]);
        exit(1);
    }

    int opt = 1;

    // Parse command line arguments
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int max_clients = atoi(argv[3]);

    // Create a socket for the server
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(1);
    }

    if( setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
        perror("setsockopt");
        exit(1);
    }

    // Set up the server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    struct user_data clients[max_clients];
    int client_count = 0;

    // Bind the socket to the server address
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(listen_fd, max_clients) == -1) {
        perror("listen");
        exit(1);
    }

    // Initialize the set of active sockets
    fd_set read_fds, master;
    FD_ZERO(&read_fds);
    FD_ZERO(&master);
    FD_SET(listen_fd, &master);
    int fdmax = listen_fd;

    int q;
	for(q = 0; q <= max_clients; q++){
		clients[q].username = '\0';
		clients[q].socket = -1;
        clients[q].user_number = 0;
	}
	
	int fcur = 0;

    while (1) {
        FD_ZERO(&read_fds)
        read_fds = master;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        for (fcur = 0; fcur <= fdmax; fcur++) {
            if (FD_ISSET(fcur, &read_fds)) {
                if (fcur == listen_fd) {
                    // Accept a new client connection
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        // Add the new client to the set

                        if(client_count+1 > max_clients){
                            sbcp_message_t* nak_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                            nak_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                            nak_msg->sMsgHeader.uiType = NAK;
                            nak_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen("Chat room full, try later\0");
                            nak_msg->sMsgAttribute.uiType = REASON;
                            nak_msg->sMsgAttribute.uiLength = strlen("Chat room full, try later\0");
                            strcpy(nak_msg.sMsgAttribute.acPayload, "Chat room full, try later\0");
                            send(new_fd, nak_msg, sizeof(sbcp_message_t), 0);
                            close(new_fd);
                            free(nak_msg)
                            break;
                        }


                        // Handle JOIN message from the new client
                        sbcp_message_t* join_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));

                        if (recv(new_fd, (sbcp_message_t*)join_msg, sizeof(sbcp_message_t), 0) == -1) {
                            perror("recv");
                        } else if (join_msg->sMsgHeader.uiType == JOIN) {
                            // Check if the username is already in use
                            bool username_exists = false;
                            for (int j = 0; j < client_count; j++) {
                                if (strcmp(clients[j].user_name, join_msg.sMsgAttribute.acPayload) == 0) {
                                    username_exists = true;
                                    break;
                                }
                            }

                            if (!username_exists) {
                                // Add the client to the list of connected clients
                                struct user_data new_user;
                                strcpy(new_user.user_name, join_msg.sMsgAttribute.acPayload);
                                new_user.socket_fd = new_fd;
                                new_user.user_number = client_count;
                                clients[client_count] = new_user;
                                client_count++;

                                // Send ACK to the new client
                                // ## modify to send list of all users
                                sbcp_message_t* ack_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                                ack_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                                ack_msg->sMsgHeader.uiType = ACK;
                                ack_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen("Join Successful");
                                ack_msg->sMsgAttribute.uiType = MESSAGE;
                                ack_msg->sMsgAttribute.uiLength = strlen("Join Successful");
                                strcpy(ack_msg->sMsgAttribute.acPayload, "Join Successful");
                                send(new_fd, ack_msg, sizeof(sbcp_message_t), 0);
                                free(ack_msg)

                                // Notify all clients about the new user
                                char online_message_payload[512];
                                sprintf(online_message_payload, "[%s has joined the chat]\0", new_user.user_name);

                                sbcp_message_t* online_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                                online_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                                online_msg->sMsgHeader.uiType = ONLINE;
                                online_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(online_message_payload);
                                online_msg->sMsgAttribute.uiType = USERNAME;
                                online_msg->sMsgAttribute.uiLength = strlen(online_message_payload);
                                strcpy(fwd_msg.sMsgAttribute.acPayload, online_message_payload );

                                int temp_iter;
                                for (temp_iter = 0; temp_iter < max_clients; temp_iter++) {
                                    if (clients[temp_iter].socket_fd != -1 && clients[temp_iter].socket_fd != new_fd) {
                                        send(clients[temp_iter].socket_fd, &fwd_msg, sizeof(sbcp_message_t), 0);
                                    }

                                FD_SET(new_fd, &read_fds);
                                if (new_fd > fdmax) {
                                    fdmax = new_fd;
                                }
                                free(online_msg)    

                            } else {
                                // Send NAK to the new client if the username is already in use
                                sbcp_message_t nak_msg;
                                nak_msg.sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                                nak_msg.sMsgHeader.uiType = NAK;
                                nak_msg.sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen("Username already in use");
                                nak_msg.sMsgAttribute.uiType = REASON;
                                nak_msg.sMsgAttribute.uiLength = strlen("Username already in use");
                                strcpy(nak_msg.sMsgAttribute.acPayload, "Username already in use");
                                send(new_fd, &nak_msg, sizeof(sbcp_message_t), 0);
                                close(new_fd);
                            }
                        }
                    }
                } else {
                    // Handle messages (SEND or IDLE) from clients
                    sbcp_message_t recv_msg;
                    int nbytes = recv(fcur, &recv_msg, sizeof(sbcp_message_t), 0);

                    if (nbytes <= 0) {
                        // Connection closed or error occurred
                        if (nbytes == 0) {
                            // Connection closed by client
                            printf("[%s has left the chat]", clients[i].user_name);
                        } else {
                            perror("recv");
                        }
                        close(fcur);
                        FD_CLR(fcur, &master);

                        char offline_message_payload[512];
                        sprintf(offline_message, "[%s has left the chat]\n", new_user.user_name);

                        sbcp_message_t offline_message;
                        fwd_msg.sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                        fwd_msg.sMsgHeader.uiType = OFFLINE;
                        fwd_msg.sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(offline_message_payload);
                        fwd_msg.sMsgAttribute.uiType = USERNAME;
                        fwd_msg.sMsgAttribute.uiLength = strlen(offline_message_payload);
                        strcpy(fwd_msg.sMsgAttribute.acPayload, offline_message_payload);

                        for (int i = 0; i < max_clients; i++) {
                            if (clients[i].socket_fd != -1 && clients[i].socket_fd != fcur) {
                                send(clients[i].socket_fd, &fwd_msg, sizeof(sbcp_message_t), 0);
                            }
                        }    

                        // Remove the client from the list of connected clients
                        for (int j = 0; j < client_count; j++) {
                            if (clients[j].socket_fd == i) {
                                clients[j].socket_fd = -1; // Mark as inactive
                                break;
                            }
                        }
                    } else {
                        // Handle SEND or IDLE message
                        if (recv_msg.sMsgHeader.uiType == SEND) {
                            // Broadcast the message to all other clients
                            char send_message[512];
                            snprintf(send_message, sizeof(send_message), "[%s]: %s", clients[i].user_name, recv_msg.sMsgAttribute.acPayload);

                            sbcp_message_t fwd_msg;
                            fwd_msg.sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                            fwd_msg.sMsgHeader.uiType = FWD;
                            fwd_msg.sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(message);
                            fwd_msg.sMsgAttribute.uiType = MESSAGE;
                            fwd_msg.sMsgAttribute.uiLength = strlen(message);
                            strcpy(fwd_msg.sMsgAttribute.acPayload, message);

                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                if (clients[i].socket_fd != -1 && clients[i].socket_fd != sender_fd) {
                                    send(clients[i].socket_fd, &fwd_msg, sizeof(sbcp_message_t), 0);
                                }
                            }

                        } else if (recv_msg.sMsgHeader.uiType == IDLE) {
                            // Handle IDLE message
                            // You can implement idle functionality here
                            sbcp_message_t fwd_msg;
                            fwd_msg.sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                            fwd_msg.sMsgHeader.uiType = FWD;
                            fwd_msg.sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(message);
                            fwd_msg.sMsgAttribute.uiType = MESSAGE;
                            fwd_msg.sMsgAttribute.uiLength = strlen(message);
                            strcpy(fwd_msg.sMsgAttribute.acPayload, message);

                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                if (clients[i].socket_fd != -1 && clients[i].socket_fd != sender_fd) {
                                    send(clients[i].socket_fd, &fwd_msg, sizeof(sbcp_message_t), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    }
    close(listen_fd);
    return 0;
    
