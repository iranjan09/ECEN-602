#include "include.h"

void zombie_handler_func(int sigtemp_fd)
{
    wait(NULL);
}



// From UNP, vol 1, 1st Ed, p. 499]
FILE * temp_netascii_file(FILE *fd, char * file_net){
    srand(time(0));
    int n = rand();
    sprintf(file_net, "temp_file_%d.txt", n);
    FILE * temp_fd = fopen(file_net, "w");
    int c;
    int nextChar = -1;
    while(1){
        c = getc(fd);
        if(nextChar >= 0){
            fputc(nextChar, temp_fd);
            nextChar = -1;
        }
        if(c == EOF){
            if(ferror(temp_fd)){
                perror("Error writing to Temp File");
            }
            break;
        }
        else if(c == '\n'){
            c = '\r';
            nextChar = '\n';
        }
        else if(c == '\r'){
            nextChar = '\0';
        }
        else{
            nextChar = -1;
        }
        fputc(c,temp_fd);
    }
    fclose(temp_fd);
    return fopen(file_net, "r");
}

ssize_t send_tftp_data(int sock_fd, uint16_t block_num, int mode, uint8_t *data, ssize_t data_length, struct sockaddr_in *addr, socklen_t socklen) {
    
    // Initialize a TFTP data message
    tftp_data_message_t data_message;
    data_message.opCode = htons(DATA_EVENT);
    data_message.block_num = htons(block_num);
    memcpy(data_message.data, data, data_length);
    
	ssize_t block_sent = sendto(sock_fd, &data_message, sizeof(data_message), 0, (struct sockaddr *) addr, socklen);
	if(block_sent < 0)
	{
		perror("Error: Block Data Sending failed:");
	}
	return block_sent;
}

ssize_t send_error_message(int sock_fd, int ec, char * error, struct sockaddr_in * addr, socklen_t socketlen)
{
	tftp_err_t err_msg;
	if(strlen(error_data) >= FILE_SIZE){
		return 0;
	}
	err_msg.opCode = htons(ERROR_EVENT);
	err_msg.err_code = htons(ec);
	strcpy((char *)err_msg.err_data, error);
    ssize_t err_sent = sendto(sock_fd, &err_msg, sizeof(err_msg)+1, 0, (struct sockaddr *) addr, socklen);
	
	if(err_sent < 0)
	{
		perror("Error: Error Data Sending failed:");
	}
	return err_sent;
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

ssize_t tftp_recv(int sockfd, char * recv_buf, struct sockaddr_in * cli_addr, socklen_t * socklen){
	ssize_t msglen = recvfrom(sockfd, recv_buf, sizeof(*recv_buf), 0, (struct sockaddr *) cli_addr, socklen);
	if(msg_size < 0 && errno !=EAGAIN){
		perror("Error receiving message: ");
	}
	return msglen;
}

void handle_tftp_request(char *buf, ssize_t msglen, struct sockaddr_in *cli_addr, socklen_t socket_len, uint16_t opCode)
{
    ssize_t read_len, recvlen;
    char *filename, *transfer_mode;
    FILE *file_descriptor, *temporary_file_fd;
    struct timeval timeout;
	char recv_buf[MAX_BUF];
    uint16_t request_opcode, block_number = 0;
    int mode =0, retry_count, flag = 0;    
    char temp_file[50];
	int socket_fd;
	uint8_t data_buffer[512];


    if (recv_buf_parse(buf, msglen, file_name, transfer_mode) != 0) {
    printf("Parsing of received buffer failed\n");
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

    printf("Received a new connection from %s:%u!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
	
	if (strcasecmp(transfer_mode, "netascii") == 0)
        mode = NETASCII;
    else if (strcasecmp(transfer_mode, "octet") == 0)
        mode = OCTET;
	else {
		printf("Client %s:%u sent invalid mode\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
		send_error_message(socket_fd, 0, "Invalid mode", cli_addr, socket_len);
		exit(-1);
	}
	
	 file_descriptor = fopen(filename, opCode == RRQ_OPCODE ? "r" : "w");
    if (file_descriptor == NULL)
    {
        printf("File does not exist\n");
        send_error_message(socket_fd, 1, "File not found", cli_addr, socket_len);
        exit(-1);
    }
	
	   printf("File name %s in  transfer mode %s %s by %s:%u! \n", filename, mode,
            opCode == RRQ_OPCODE ? "read" : "write", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));

    if (opCode == RRQ_OPCODE)
    {
        if (mode == NETASCII)
        {
            temporary_file_fd = temp_netascii_file(file_descriptor,temp_file);
            fclose (file_descriptor);
            file_descriptor = temporary_file_fd;
        }
        while (!flag)
        {
            read_len = fread(data_buffer, 1, sizeof(data_buffer), file_descriptor);
            block_number++;

            if (read_len < 512)
            {
                flag = 1;		// terminating data block
            }

            retry_count = MAX_RETRIES;
            while (retry_count)
            {
                if (send_tftp_data(socket_fd, block_number, mode, data_buffer, read_len, cli_addr, socket_len) < 0)
                {
                    printf("TFTP terminated for client %s:%u!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(-1);
                }

                recvlen = tftp_recv(socket_fd, &recv_buf, cli_addr, socket_len);

                if (recvlen >= 0 && recvlen < 4)
                {
                    printf("Invalid req size received from %s:%u\n",inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    send_error_message(socket_fd, 0, "Invalid req size", cli_addr, socket_len);
                    exit(-1);
                }

                if (recvlen >= 4)
                {
                    break;
                }

                if (errno != EAGAIN)
                {
                    printf("Client %s:%u session terminated.\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(-1);
                }

                printf("Client %s:%u not responding. Trying again.\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                retry_count--;
            }

            if (retry_count == 0)
            {
                printf("Retries failed. Transfer ended.\n");
                exit(-1);
            }
			
		    tftp_req_t tftp_req = (tftp_header_t *)recv_buf;
		    opCode = ntohs(tftp_req.opCode)
			
			if (opCode != ERROR_EVENT && opCode!= ACK_EVENT)
            {
                printf("Client %s:%u sent unknown message\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                //send_error_message(socket_fd, 0, "unexpected message received", cli_addr, socket_len);
                exit(-1);
            }
			

            if (opCode == ERROR_EVENT)
			
            {
				tftp_err_t error = (tftp_error_t *)recv_buf;
                printf("Client %u:%s has send error message %u:%s\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port), ntohs(error.err_code), error.err_data);
                exit(-1);
            }

            if (opCode == ACK_EVENT)
		    {
				tftp_ack_t ack_msg = (tftp_ack_t *) recv_buf
				if (ack_msg.block_num != block_number) {
                         
                printf("Client %s:%u send invalid block ack\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                //send_error_message(socket_fd, 0, "Server:Acknowledgment number invalid", cli_addr, socket_len);
                exit(-1);
				}
            }
        }
    }
	
	printf("TFTP data sent successfully to %s:%u!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
    fclose(file_descriptor);
    if(temp_file != NULL){
        remove(temp_file);
    }
    close(socket_fd);
    exit(0);

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
