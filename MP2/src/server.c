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
		strncpy(clients[q].user_name,"\0",sizeof("\0"));
		clients[q].socket_fd = -1;
        clients[q].user_number = 0;
	}
	
	int fcur = 0;

    printf("Server intialized\n");

    while (1) {
        FD_ZERO(&read_fds);
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

                        if(client_count + 1 > max_clients){
                            sbcp_message_t* nak_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                            nak_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                            nak_msg->sMsgHeader.uiType = NAK;
                            nak_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen("Chat room full, try later\0");
                            nak_msg->sMsgAttribute.uiType = REASON;
                            nak_msg->sMsgAttribute.uiLength = strlen("Chat room full, try later\0");
                            strncpy(nak_msg->sMsgAttribute.acPayload, "Chat room full, try later\0", sizeof("Chat room full, try later\0"));
                            send(new_fd, nak_msg, sizeof(sbcp_message_t), 0);
                            close(new_fd);
                            free(nak_msg);
                            break;
                        }


                        // Handle JOIN message from the new client
                        sbcp_message_t* join_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));

                        if (recv(new_fd, (sbcp_message_t*)join_msg, sizeof(sbcp_message_t), 0) == -1) {
                            perror("recv");
                        } else if (join_msg->sMsgHeader.uiType == JOIN) {
                            // Check if the username is already in use
                            bool username_exists = false;
                            int j = 0;
                            for (j = 0; j < client_count; j++) {
                                if (strcmp(clients[j].user_name, join_msg->sMsgAttribute.acPayload) == 0) {
                                    username_exists = true;
                                    break;
                                }
                            }

                            if (!username_exists) {
                                // Add the client to the list of connected clients
                                struct user_data new_user;
                                strncpy(new_user.user_name, join_msg->sMsgAttribute.acPayload,sizeof(join_msg->sMsgAttribute.acPayload));
                                new_user.socket_fd = new_fd;
                                new_user.user_number = client_count+1;
                                clients[client_count] = new_user;
                                client_count++;

                                // Send ACK to the new client
                                // ## modify to send list of all users
                                sbcp_message_t* ack_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                                ack_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                                ack_msg->sMsgHeader.uiType = ACK;
                                ack_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen("Join Successful\0");
                                ack_msg->sMsgAttribute.uiType = MESSAGE;
                                ack_msg->sMsgAttribute.uiLength = strlen("Join Successful\0");
                                strncpy(ack_msg->sMsgAttribute.acPayload, "Join Successful\0",sizeof("Join Successful\0"));
                                send(new_fd, ack_msg, sizeof(sbcp_message_t), 0);
                                free(ack_msg);

                                // Notify all clients about the new user
                                char online_message_payload[16];
                                printf("New user has joined the chat ->");
                                sprintf(online_message_payload, "%s\0", new_user.user_name);

                                sbcp_message_t* online_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                                online_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                                online_msg->sMsgHeader.uiType = ONLINE;
                                online_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(online_message_payload);
                                online_msg->sMsgAttribute.uiType = USERNAME;
                                online_msg->sMsgAttribute.uiLength = strlen(online_message_payload);
                                strcpy(online_msg->sMsgAttribute.acPayload, online_message_payload );

                                int temp_iter;
                                for (temp_iter = 0; temp_iter < max_clients; temp_iter++) {
                                    if (clients[temp_iter].socket_fd != -1 && clients[temp_iter].socket_fd != new_fd) {
                                        send(clients[temp_iter].socket_fd, online_msg, sizeof(sbcp_message_t), 0);
                                    }
                                free(online_msg);   

                                FD_SET(new_fd, &master);
                                if (new_fd > fdmax) {
                                    fdmax = new_fd;
                                }
                                free(online_msg);
                                free(join_msg);    

                            }} else {
                                // Send NAK to the new client if the username is already in use
                                sbcp_message_t* nak_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                                nak_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                                nak_msg->sMsgHeader.uiType = NAK;
                                nak_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen("Username already in use\0");
                                nak_msg->sMsgAttribute.uiType = REASON;
                                nak_msg->sMsgAttribute.uiLength = strlen("Username already in use\0");
                                strncpy(nak_msg->sMsgAttribute.acPayload, "Username already in use\0",sizeof("Username already in use\0"));
                                send(new_fd, nak_msg, sizeof(sbcp_message_t), 0);
                                free(nak_msg);
                                free(join_msg);
                                close(new_fd);
                            }
                        }
                    }
                } else {
                    // Handle messages (SEND or IDLE) from clients
                    sbcp_message_t* recv_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                    int nbytes = recv(fcur, recv_msg, sizeof(sbcp_message_t), 0);

                    int i=0;
                        int client_index;
                        for (i = 0; i < max_clients; i++) {
                            if (clients[i].socket_fd == fcur) {
                                client_index = i;
                            }
                        }

                    if (nbytes <= 0) {
                        // Connection closed or error occurred
                    
                        if (nbytes == 0) {
                            printf("[%s has left the chat]", clients[client_index].user_name);
                        } else {
                            perror("recv");
                        }
                        close(fcur);
                        FD_CLR(fcur, &master);

                        char offline_message_payload[512];
                        struct user_data new_user;
                        
                        sprintf(offline_message_payload, "%s\0", new_user.user_name);

                        sbcp_message_t* offline_message = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                        offline_message->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                        offline_message->sMsgHeader.uiType = OFFLINE;
                        offline_message->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(offline_message_payload);
                        offline_message->sMsgAttribute.uiType = USERNAME;
                        offline_message->sMsgAttribute.uiLength = strlen(offline_message_payload);
                        strncpy(offline_message->sMsgAttribute.acPayload, offline_message_payload, sizeof(offline_message_payload));

                        i = 0;
                        for (i = 0; i < max_clients; i++) {
                            if (clients[i].socket_fd != -1 && clients[i].socket_fd != fcur) {
                                send(clients[i].socket_fd, offline_message, sizeof(sbcp_message_t), 0);
                            }
                        }    

                        close(clients[client_index].socket_fd);
                        // Remove the client from the list of connected clients
                        clients[client_index].socket_fd = -1;
                        strncpy(clients[client_index].user_name,"\0",sizeof("\0"));
                        clients[client_index].user_number = 0;
                        client_count--;
                        
                        //Reassign user numbers
                        int j = 0;
                        int k = 0;
                        for (j = 0; j < client_count; j++) {
                            if (clients[j].socket_fd != -1 ) {
                                clients[j].user_number = k+1;
                                k++;
                            }        
                        }

                    } else {
                        // Handle SEND or IDLE message
                        if (recv_msg->sMsgHeader.uiType == SEND) {
                            // Broadcast the message to all other clients
                            char send_message_payload[512];
                            sprintf(send_message_payload, "[%s]: %s", clients[client_index].user_name, recv_msg->sMsgAttribute.acPayload);

                            sbcp_message_t* fwd_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                            fwd_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                            fwd_msg->sMsgHeader.uiType = FWD;
                            fwd_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(send_message_payload);
                            fwd_msg->sMsgAttribute.uiType = MESSAGE;
                            fwd_msg->sMsgAttribute.uiLength = strlen(send_message_payload);
                            strncpy(fwd_msg->sMsgAttribute.acPayload, send_message_payload,sizeof(send_message_payload));
                            
                            i = 0;
                            for (i = 0; i < max_clients; i++) {
                                if (clients[i].socket_fd != -1 && clients[i].socket_fd != fcur) {
                                    send(clients[i].socket_fd, fwd_msg, sizeof(sbcp_message_t), 0);
                                }
                            }
                            free(fwd_msg);

                        } else if (recv_msg->sMsgHeader.uiType == IDLE) {
                            // Handle IDLE message
                            char idle_message_payload[16];
                            sprintf(idle_message_payload, "%s\0", clients[i].user_name);

                            sbcp_message_t* idle_msg = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
                            idle_msg->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                            idle_msg->sMsgHeader.uiType = FWD;
                            idle_msg->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(idle_message_payload);
                            idle_msg->sMsgAttribute.uiType = USERNAME;
                            idle_msg->sMsgAttribute.uiLength = strlen(idle_message_payload);
                            strcpy(idle_msg->sMsgAttribute.acPayload, idle_message_payload);

                            i = 0;
                            for (i = 0; i < max_clients; i++) {
                                if (clients[i].socket_fd != -1 && clients[i].socket_fd != fcur) {
                                    send(clients[i].socket_fd, idle_msg, sizeof(sbcp_message_t), 0);
                                }
                            }
                            free(idle_msg);
                        }
                    }
                }
            }
        }
    }
    
    close(listen_fd);
    return 0;
    }
    
