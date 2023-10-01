#include "common.h"

pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;
bool isIdleMessageSent = false;


/*int create_socket(bool isIPv4)
{
    int socket_fd = -1;
    if (isIPv4 ==  true)
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    else
    {
        socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    }
    if(socket_fd == -1)
    {
        perror("Socket Create Faild");
        exit(-1);
    }
    else{
        printf("Socket is Created Successfully....\n");
    }
    return socket_fd;
}*/

void send_chat_message(int socket_fd, const char* message) {
	
    sbcp_message_t* chatMessage = (sbcp_message_t*)calloc(1, sizeof(sbcp_message_t));
    
    if (chatMessage == NULL) {
        perror("ERROR: Memory allocation for chat message failed");
        return;
    }

    // SBCP Header - SEND
    chatMessage->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
    chatMessage->sMsgHeader.uiType = SEND;
    chatMessage->sMsgHeader.uiLength = sizeof(sbcp_attribute_t) + strlen(message);
	//printf("SEND MESSAGE\n");
    // SBCP Attribute - MESSAGE
    chatMessage->sMsgAttribute.uiType = MESSAGE;
    chatMessage->sMsgAttribute.uiLength = strlen(message);
    strncpy(chatMessage->sMsgAttribute.acPayload, message, sizeof(chatMessage->sMsgAttribute.acPayload) - 1);
    chatMessage->sMsgAttribute.acPayload[sizeof(chatMessage->sMsgAttribute.acPayload) - 1] = '\0';

    pthread_mutex_lock(&ready_mutex);
    pthread_cond_signal(&ready_cond);
    isIdleMessageSent = false;
    pthread_mutex_unlock(&ready_mutex);

    int send_result = send(socket_fd, chatMessage, sizeof(sbcp_message_t), 0);

    if (send_result < 0) {
        perror("ERROR: Write failed to send chat message");
    }
	//printf("mESSAGE SENT\n");
    free(chatMessage);
}


void handle_server_messages(int server_socket) {
	//printf("server sent some\n");
    sbcp_message_t* message = NULL;
    int recv_result = 0;

    //while (1) {
        message = (sbcp_message_t*)calloc(1, sizeof(sbcp_message_t));
        if (message == NULL) {
            perror("ERROR: Memory allocation for message failed");
            return;
        }

        recv_result = recv(server_socket, (sbcp_message_t*)message, sizeof(sbcp_message_t), 0);

        if (recv_result == -1) {
            perror("ERROR: Failed to receive server message");
            free(message);
            return;
        } else if (recv_result == 0) {
            // Server has closed the connection
            printf("Server has closed the connection\n");
            free(message);
            return;
        }

        // Check the received message type
        switch (message->sMsgHeader.uiType) {
            case FWD:
            case ACK:
                // Check the attribute type
                if (message->sMsgAttribute.uiType == MESSAGE) {
                    // FWD/ACK Message received. Display the message
                    if (message->sMsgAttribute.acPayload != NULL && message->sMsgAttribute.acPayload[0] != '\0') {
                        printf("%s\n", message->sMsgAttribute.acPayload);
                    }
                }
                break;

            case NAK:
                // Check the attribute type
                if (message->sMsgAttribute.uiType == REASON) {
                    // NAK Message received. Display the reason
                    if (message->sMsgAttribute.acPayload != NULL && message->sMsgAttribute.acPayload[0] != '\0') {
                        printf("NAK received from the server: %s\n", message->sMsgAttribute.acPayload);
                        free(message);
                        exit(2);
                    }
                }
                break;

            default:
                // Handle other message types if needed
                break;
        }

        free(message);
   // }
}

void handle_user_input(int server_socket) {
    
    char chat_message[PAYLOAD_SIZE];
    unsigned int message_length;
	//printf("The std inppp\n");
            memset(chat_message, 0, PAYLOAD_SIZE);
            if (fgets(chat_message, PAYLOAD_SIZE, stdin) == NULL) {
                perror("ERROR: Failed to read user input");
                exit(EXIT_FAILURE);
            }

            // Remove trailing newline if present
            message_length = strlen(chat_message);
            if (message_length > 0 && chat_message[message_length - 1] == '\n') {
                chat_message[message_length - 1] = '\0';
            }
			//printf("iNPUT MESSAGE \n");
            // Send the chat message to the server
            send_chat_message(server_socket, chat_message);
        
    
}


void send_join_message(int socket_fd, const char* user_name) {
    sbcp_message_t* message = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));
    if (message == NULL) {
        perror("ERROR: Memory allocation failed");
        return;
    }

    // Fill in the SBCP Header - JOIN
    message->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
    message->sMsgHeader.uiType = JOIN;
    message->sMsgHeader.uiLength = 20;

    // Fill in the SBCP Attribute - USERNAME
    message->sMsgAttribute.uiType = USERNAME;
    message->sMsgAttribute.uiLength = (4 + strlen(user_name));
    if (message->sMsgAttribute.uiLength > PAYLOAD_SIZE) {
        perror("ERROR: User name is too long");
        free(message);
        return;
    }
    strncpy(message->sMsgAttribute.acPayload, user_name, sizeof(message->sMsgAttribute.acPayload) - 1);
    message->sMsgAttribute.acPayload[sizeof(message->sMsgAttribute.acPayload) - 1] = '\0';

    // Send the JOIN message to the server
    int send_result = send(socket_fd, (sbcp_message_t *)message, sizeof(sbcp_message_t), 0);
    if (send_result < 0) {
        perror("ERROR: Write failed to send JOIN message");
    } else {
        printf("The JOIN message has been sent successfully\n");
    }

    free(message);
}

// Function to determine whether the server address is IPv4 or IPv6
int get_server_address_type(const char* server_address) {
	
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;

    // Try to parse as IPv4 address
    if (inet_pton(AF_INET, server_address, &ipv4_addr) == 1) {
        return AF_INET; // IPv4 address
    }
    // Try to parse as IPv6 address
    else if (inet_pton(AF_INET6, server_address, &ipv6_addr) == 1) {
        return AF_INET6; // IPv6 address
    } else {
        return -1; // Invalid address
    }
}

void send_idle_message(void *arg) {
	
	int *socket_fd = (int *)arg;
    int ret = 0;
    time_t current_time;
    struct timespec timeout;
    sbcp_message_t* idle_message = NULL;

    while (1) {
        time(&current_time);
        timeout.tv_sec = current_time + IDLE_WAIT_TIME;

        pthread_mutex_lock(&ready_mutex);
        ret = pthread_cond_timedwait(&ready_cond, &ready_mutex, &timeout);
        pthread_mutex_unlock(&ready_mutex);

        if (ret != 0) {
            if (errno == EAGAIN) {
                // Handle the case where the condition variable was signaled but not due to a timeout
            } else {
                if (isIdleMessageSent == false) {
                    isIdleMessageSent = true;

                    idle_message = (sbcp_message_t *)calloc(1, sizeof(sbcp_message_t));

                    if (idle_message == NULL) {
                        perror("ERROR: Memory allocation for idle message failed");
                        return;
                    }

                    // SBCP Header - SEND
                    idle_message->sMsgHeader.uiVrsn = PROTOCOL_VERSION;
                    idle_message->sMsgHeader.uiType = IDLE;
                    idle_message->sMsgHeader.uiLength = 520;

                    // Sending IDLE message to server
                    ret = send(*socket_fd, (sbcp_message_t *)idle_message, sizeof(sbcp_message_t), 0);

                    if (ret < 0) {
                        perror("ERROR: Write failed to send IDLE message");
                    }

                    free(idle_message);
                }
            }
        }
    }
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("ERROR: Please provide User name, IP address, and port number\n");
        return 1;
    }

    int iSocket_fd = -1, port_number = -1;
    char* pcUserName, * pcIpAddress;
    fd_set fdRead_Set;
    int fdMax;
    sbcp_message_t* psMessage = NULL;
    unsigned int uiChatLength;
    pthread_attr_t sThreadAttr;
    pthread_t iThreadId = 0;
    struct sockaddr_storage server_address;
    int server_address_type;
    int ret;
	fd_set read_fds;
    int max_fd;
	
    // Command line argument parsing and initialization...
    port_number = atoi(argv[3]);
    pcIpAddress = argv[2];
    pcUserName = argv[1];

    // Determine the server address type (IPv4 or IPv6)
    server_address_type = get_server_address_type(pcIpAddress);

    if (server_address_type == -1) {
        printf("Invalid server address: %s\n", pcIpAddress);
        return 1;
    }

    // Print user information...
    printf("CLIENT: User Name is %s\n", pcUserName);
    printf("CLIENT: IP Address is %s (Type: %s)\n", pcIpAddress, (server_address_type == AF_INET) ? "IPv4" : "IPv6");
    printf("CLIENT: Port number is %d\n", port_number);

    // Set up socket and connect...
    if (server_address_type == AF_INET) {
        iSocket_fd = create_socket(true);
		struct sockaddr_in ipv4_server;
        memset(&ipv4_server, 0, sizeof(struct sockaddr_in));
        ipv4_server.sin_family = AF_INET;
        ipv4_server.sin_port = htons(port_number);
		ret = inet_pton (AF_INET, pcIpAddress, &(ipv4_server.sin_addr));
        if (ret < 0)
            perror ("ERROR: IP conversion failed");

        /* Connect to server */
        ret = connect (iSocket_fd, (struct sockaddr *)&ipv4_server, sizeof(ipv4_server));
        if (ret < 0)
            perror ("ERROR: Socket connection failed");
    } else if (server_address_type == AF_INET6) {
        iSocket_fd = create_socket(false);
        struct sockaddr_in6 ipv6_server;
        memset(&ipv6_server, 0, sizeof(struct sockaddr_in6));
        ipv6_server.sin6_family = AF_INET6;
        ipv6_server.sin6_port = htons(port_number);
        ret = inet_pton(AF_INET6, pcIpAddress, &(ipv6_server.sin6_addr));
		if (ret < 0)
            perror ("ERROR: IP conversion failed");

        /* Connect to server */
        ret = connect (iSocket_fd, (struct sockaddr *)&ipv6_server, sizeof(ipv6_server));
        if (ret < 0)
            perror ("ERROR: Socket connection failed");
    } else {
		printf("Invalid server address: %s\n", pcIpAddress);
        return 1;
    }
	
	send_join_message(iSocket_fd, pcUserName);
	
	FD_ZERO(&read_fds);
	
	FD_SET(STDIN_FILENO, &read_fds);      // Add standard input (keyboard) to the read_fds set
    FD_SET(iSocket_fd, &read_fds);     // Add server socket to the read_fds set
    

    pthread_attr_init(&sThreadAttr);
    pthread_attr_setdetachstate(&sThreadAttr, PTHREAD_CREATE_DETACHED);

    int iRet = pthread_create((pthread_t*)&iThreadId, &sThreadAttr, 
								(void *)&send_idle_message, 
								(void*)&iSocket_fd);

	while(1)
    {
        max_fd = iSocket_fd;
        if (select (max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("ERR: Select Error");
            exit(6);
        }

        if (FD_ISSET(0, &read_fds))
        {
			//printf("The std in\n");
			
			handle_user_input(iSocket_fd);
		}
		if (FD_ISSET(iSocket_fd, &read_fds))
        {
			//printf("server sent some\n");
        handle_server_messages(iSocket_fd);
		}
		
		FD_SET(STDIN_FILENO, &read_fds);      // Add standard input (keyboard) to the read_fds set
		FD_SET(iSocket_fd, &read_fds);     // Add server socket to the read_fds set
	}
    return 0;
}
