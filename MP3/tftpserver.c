#include "include.h"

void handle_zombie(int sigtemp_fd)
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

ssize_t transmitData(int server_fd, mess_recv_t * msg, size_t len, struct sockaddr_in *addr, socklen_t socklen){
	ssize_t sent = sendto(server_fd, msg, len, 0, (struct sockaddr *) addr, socklen);
	if(sent < 0)
	{
		perror("Data sending to client failed:");
	}
	return sent;
}

ssize_t send_tftp_data(int sock_fd, struct sockaddr_in *addr, socklen_t socklen, uint16_t block_num, int mode, uint8_t *data, ssize_t data_length) {
    
    // Initialize a TFTP data message
    mess_recv_t data_message;
	
	char msg[MAX_BUF];	
	int tot = 0;
	int extra = 0;
		
	memcpy(msg, data, data_length);
	tot = extra + data_length;
	
    data_message.opCode = htons(DATA_EVENT);
    data_message.tftp_data.block_num = htons(block_num);
	
    memcpy(data_message.tftp_data.data, msg, tot);
	return transmitData(sock_fd, &data_message, tot+4, addr, socklen);
}

ssize_t send_tftp_error(int sock_fd, int ec, char * error, struct sockaddr_in * addr, socklen_t socketlen)
{
	mess_recv_t err_msg;
	if(strlen(error) >= MAX_BUF){
		return 0;
	}
	err_msg.tftp_err.opCode = htons(ERR_EVENT);
	err_msg.tftp_err.err_code = htons(ec);
	strcpy((char *)err_msg.tftp_err.err_data, error);
	
	return transmitData(sock_fd, &err_msg, strlen(error)+5, addr, socketlen);
}


ssize_t tftp_recv(int sockfd, struct sockaddr_in *cli_addr, socklen_t * socklen, mess_recv_t *recv_buf){
	ssize_t msglen = recvfrom(sockfd, recv_buf, sizeof(*recv_buf), 0, (struct sockaddr *) cli_addr, socklen);
	if(msglen < 0 && errno !=EAGAIN){
		perror("Error in tftp receive: ");
	}
	return msglen;
}

void handle_tftp_request(struct sockaddr_in *cli_addr, socklen_t socket_len, mess_recv_t *buf, ssize_t len, uint16_t opCode)
{
    ssize_t read_len, recvlen;
    char *filename, *null_terminator_position, *transfer_mode;
	FILE *file_descriptor, *temporary_file_fd;
    struct timeval timeout;
	uint16_t block_number = 0;
	//uint16_t recv_opCode;
    int mode = 0, retryAttempts, flag = 0;    
    char temp_file[50];
	int server_fd;
	uint8_t data_buffer[512];
	mess_recv_t client_msg;
	
	
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed. Exit!");
        exit(EXIT_FAILURE);
    }

    timeout.tv_usec = 0;
    timeout.tv_sec = TIMEOUT;

    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Setting socket timeoit failed");
        exit(EXIT_FAILURE);
    }

    printf("Client %s:%u Connected to TFTP server\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
	
	// Cast the buffer to a the request structure to get filename
    filename = (char *)buf->tftp_req.file_mode;

    // Locate the null terminator (0) in the trailing buffer to separate filename and mode
    null_terminator_position = &filename[len - 3];
	
	// Check if the null terminator was found
	if (*null_terminator_position != '\0') {
        printf("Requested invalid file name or mode incorrect\n");
		send_tftp_error(server_fd, UNDEFINED, "Filename invalid or transfermode incorrect", cli_addr, socket_len);
        exit(EXIT_FAILURE);
    }
	
	//Find the mode
	transfer_mode = strchr(filename, '\0') + 1;
 
    if (transfer_mode > null_terminator_position) {
        perror("Transfer mode not given");
		send_tftp_error(server_fd, UNDEFINED, "Filename invalid or transfermode incorrect", cli_addr, socket_len);
        exit(EXIT_FAILURE);
    }
	
	
	if (strcasecmp(transfer_mode, "netascii") == 0)
        mode = NETASCII;
    else if (strcasecmp(transfer_mode, "octet") == 0)
        mode = OCTET;
	else
		mode = 0;

    // Print the parsed filename and mode
    printf("Requested file: %s, Mode: %s\n", filename, transfer_mode);
	

	if (mode == 0) {
		printf("Client %s:%u sent invalid mode\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
		send_tftp_error(server_fd, UNDEFINED, "Transfer mode invalid", cli_addr, socket_len);
		exit(EXIT_FAILURE);
	}
	
	file_descriptor = fopen(filename, opCode == RRQ_EVENT ? "r" : "w");
    if (file_descriptor == NULL)
    {
        printf("File does not exist\n");
        send_tftp_error(server_fd, FILE_NOT_FOUND, "File not found", cli_addr, socket_len);
        exit(EXIT_FAILURE);
    }

	printf("File name %s in  transfer mode %d %s request by %s:%u! \n", filename, mode,
            opCode == RRQ_EVENT ? "read" : "write", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));

    if (opCode == RRQ_EVENT)
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

            if (read_len < MAX_BUF)
            {
                flag = 1;
            }

            retryAttempts = RETRY_LIMIT;
            while (retryAttempts)
            {
                if (send_tftp_data(server_fd, cli_addr, socket_len, block_number, mode, data_buffer, read_len) < 0)
                {
                    printf("Client %s:%u TFTP session terminated\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(EXIT_FAILURE);
                }

                recvlen = tftp_recv(server_fd, cli_addr, &socket_len, &client_msg);

                if (recvlen >= 0 && recvlen < MIN_FRAME_LEN)
                {
					printf("Invalid request from %s:%d. Try again!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
					send_tftp_error(server_fd, UNDEFINED, "Frame size is invalid", cli_addr, socket_len);
                    exit(EXIT_FAILURE);
                }

                if (recvlen >= MIN_FRAME_LEN)
                {
                    break;
                }

                if (errno != EAGAIN)
                {
                    printf("Client %s:%u TFTP session terminated.\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(EXIT_FAILURE);
                }

                printf("Client %s:%u not responding. Trying again.\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                retryAttempts--;
            }

            if (retryAttempts == 0)
            {
                printf("Retries failed. Transfer ended.\n");
                exit(EXIT_FAILURE);
            }
			
            if (ntohs(client_msg.opCode) == ERR_EVENT)
            {
                printf("Client %s:%u sent error event message %u:%s\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port), ntohs(client_msg.tftp_err.err_code),
                        client_msg.tftp_err.err_data);
                exit(EXIT_FAILURE);
            }

            if (ntohs(client_msg.opCode) != ACK_EVENT)
            {
                printf("Client %s:%u sent unknown opcode\n",inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                send_tftp_error(server_fd, 0, "Unknown Opcode. try again", cli_addr, socket_len);
                exit(EXIT_FAILURE);
            }

            if (ntohs(client_msg.tftp_ack.block_num) != block_number) 
            {
                printf("Client %s:%u ACK number sent is high\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                send_tftp_error(server_fd, 0, "ACK Invalid", cli_addr, socket_len);
                exit(EXIT_FAILURE);
            }
        }
    }
	
	printf("TFTP data sent successfully to %s:%u!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
    fclose(file_descriptor);
    if(temp_file != NULL){
        remove(temp_file);
    }
    close(server_fd);
    exit(0);

}	


int main(int argc, char *argv[]) {
	
	if (argc < 2) {
        fprintf(stderr, "Usage: %s [directory] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	
	int sockfd;
    ssize_t recvlen;
    struct sockaddr_in serv_addr, cli_addr;
    //int portno = atoi(argv[2]);
	uint16_t opCode, portno = 0;
	socklen_t socklen = sizeof(serv_addr);
	mess_recv_t recv_buf;


	if (argc > 2) {
		if (sscanf(argv[2], "%hu", &portno)) {
            portno = htons(portno);
        }
        else
        {
            fprintf(stderr, "Server: Error Port Number Invalid\n");
            exit(-1);
        }
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("SERVER: Error opening socket");
        exit(EXIT_FAILURE);
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = portno;


    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("SERVER: Error on binding");
        exit(EXIT_FAILURE);
    }

	signal(SIGCHLD, (void *)handle_zombie);
	
    printf("SERVER: TFTP Server Initialized, waiting for clients\n");
    
    while (1) {
        recvlen = tftp_recv(sockfd, &cli_addr, &socklen, &recv_buf);
        if (recvlen < 0) {
            continue;
        }

        if (recvlen < MIN_FRAME_LEN) {
            printf("Invalid request from %s:%d. Try again!\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            send_tftp_error(sockfd, UNDEFINED, "Frame size is invalid", &cli_addr, socklen);
            continue;
        }
				  
		opCode = ntohs(recv_buf.opCode);

        if (opCode == RRQ_EVENT || opCode == WRQ_EVENT) {
            
            if (fork() == 0) {
				//Let Child proceess handle
                handle_tftp_request(&cli_addr, socklen, &recv_buf, recvlen, opCode);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Client %s:%d sent invalid request opcode\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            send_tftp_error(sockfd, UNDEFINED, "Invalid opcode", &cli_addr, socklen);
        }
    }

    close(sockfd);
    printf("Closing Server socket.\n");
    return 0;
}
