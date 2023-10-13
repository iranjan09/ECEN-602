ssize_t send_tftp_data(int sock_fd, uint16_t block_num, uint8_t *data, ssize_t data_length, struct sockaddr_in *addr, socklen_t socklen) {
    
    // Initialize a TFTP data message
    tftp_data_message_t data_message;
    data_message.uiOpcode = htons(DATA_EVENT);
    data_message.uiBlockNumber = htons(uiBlockNumber);
    memcpy(data_message.data, data, data_length);

    // Send the data message
    return send_message(sock_fd, &data_message, sizeof(data_message), addr, socklen);
}


int recv_buf_parse(char *rec_buf, ssize_t length, char *file_name, char *transfer_mode) {
    // Cast the buffer to a TFTP header structure
    tftp_req_t *tftp_req = (tftp_header_t *)req;

    // Locate the null terminator (0) in the trailing buffer to separate filename and mode
    char *null_terminator_position = strchr(tftp_req->file_mode, 0);

    // Check if the null terminator was found
    int null_terminator_index = (null_terminator_position == NULL ? -1 : null_terminator_position - tftp_req->file_mode);

    // If the null terminator was not found, indicate an error
    if (null_terminator_index < 0) {
        perror("Filename Invalid\n");
        return -1;
    }

    // Extract the filename from the trailing buffer
    memcpy(file_name, tftp_req->file_mode, null_terminator_index);

    // The rest of the buffer contains the transfer mode
    memcpy(transfer_mode, &(tftp_req->file_mode[null_terminator_index + 1]), 8 * sizeof(char));

    // Print the parsed filename and mode
    printf("Requested file: %s, Mode: %s\n", file_name, transfer_mode);

    return 0;
}

void handle_tftp_request(char *recv_buf, ssize_t msglen, struct sockaddr_in *cli_addr, socklen_t socket_len, uint16_t opCode)
{
    tftp_message_t response;
    int socket_fd;
    ssize_t data_length, send_receive_size;
    uint16_t block_number = 0;
    struct timeval timeout;
    char *filename, *transfer_mode, *end_of_request;
    FILE *file_descriptor, *temporary_file;
    int mode = 0;
    uint16_t request_opcode;
    uint8_t data_buffer[512];
    int retry_count;
    int exit_flag = 0;
    char temp_file_name[50];


    if (recv_buf_parse(recv_buf, msglen, file_name, transfer_mode) != 0) {
    printf("Parsing of recv_buf failed\n");
    return -1;
    }
	
	
	// Open a new socket to handle incoming requests
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket() fail");
        exit(-1);
    }

    timeout.tv_usec = 0;
    timeout.tv_sec = TIMEOUT;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt() fail");
        exit(-1);
    }

    printf("Received a new connection from %s:%u!\n", inet_ntoa(client_socket->sin_addr), ntohs(client_socket->sin_port));
	
	if (strcasecmp(transfer_mode, "netascii") == 0)
        mode = NETASCII;
    else if (strcasecmp(transfer_mode, "octet") == 0)
        mode = OCTET;
	else {
		printf("Transfer mode Invalid from %s:%u\n", inet_ntoa(client_addr->sin_addr), ntohs(cli_addr->sin_port));
		//send_error_message to client
		exit(-1);
	}
	
	 file_descriptor = fopen(filename, opCode == RRQ_OPCODE ? "r" : "w");
    if (file_descriptor == NULL)
    {
        printf("File does not exist\n");
        //send_error_message(socket_fd, 1, "File not found", client_socket, socket_length);
        exit(-1);
    }


}	

ssize_t tftp_recv(int sockfd, char * recv_buf, struct sockaddr_in * cli_addr, socklen_t * socklen){
	ssize_t msglen = recvfrom(sockfd, recv_buf, sizeof(*recv_buf), 0, (struct sockaddr *) cli_addr, socklen);
	if(msg_size < 0 && errno !=EAGAIN){
		perror("Error receiving message: ");
	}
	return msglen;
}

int main(int argc, char *argv[]) {
	
	if (argc < 2) {
        fprintf(stderr, "Usage: %s [base directory] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	
	int sockfd, client_sock;
    ssize_t msglen;
    struct sockaddr_in serv_addr, cli_addr;
    int portno = atoi(argv[2]);
	uint16_t opCode;
	socklen_t socklen = sizeof(server_sock);
	char recv_buf[MAX_BUF];

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP););
    if (sockfd < 0) {
        perror("SERVER: Error opening socket");
        exit(1);
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);


    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("SERVER: Error on binding");
        exit(1);
    }

    printf("SERVER: TFTP Server Initialized, waiting for clients\n");
	
    signal(SIGCHLD, (void *)zombie_handler_func);

    printf("TFTP server is running now. Listening on port: %d\n", ntohs(server_sock.sin_port);

    while (1) {
        msglen = tftp_recv(sock_fd, &recv_buf, &cli_addr, &socklen);
        if (msglen < 0) {
            continue;
        }

        if (msglen < 4) {
            printf("Invalid request from %s:%d. Try again!\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            //send_error_message(sock_fd, 0, "Invalid size", &client_sock, socklen);
            continue;
        }
		
		  tftp_req_t tftp_req = (tftp_header_t *)buf;
		  opCode = ntohs(tftp_req.opCode)


        if (opCode == RRQ_EVENT || opCode == WRQ_EVENT) {
            
            if (fork() == 0) {
				//Let Child proceess handle
                handle_tftp_request(&recv_buf, msglen, &cli_addr, socklen, opCode);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Req type invalid from %s:%d \n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            //send_error_message(sock_fd, 0, "Invalid opcode", &client_sock, socklen);
        }
    }

    close(sockfd);
    printf("Closing  Server socket.\n");
    return 0;
}