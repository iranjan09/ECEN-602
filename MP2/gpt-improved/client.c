typedef struct {
  sbcp_header_t header;
  sbcp_attribute_t attribute;
} chat_message_t;


// Create and initialize a chat message

chat_message_t* create_chat_message(const char* message_text) {

  chat_message_t* msg = calloc(1, sizeof(chat_message_t));

  if (!msg) {
    perror("Unable to allocate chat message");
    return NULL;
  }

  msg->header.version = PROTOCOL_VERSION;
  msg->header.type = SEND;
  msg->header.length = sizeof(sbcp_attribute_t) + strlen(message_text);

  msg->attribute.type = MESSAGE;
  msg->attribute.length = strlen(message_text);
  strncpy(msg->attribute.payload, message_text, MAX_PAYLOAD_SIZE);
  msg->attribute.payload[MAX_PAYLOAD_SIZE-1] = '\0';

  return msg;

}

// Send a chat message
void send_chat_message(int client_socket, const char* message_text) {

  chat_message_t* msg = create_chat_message(message_text);
  if (!msg) {
    return;
  }

  int send_result = send(client_socket, msg, sizeof(chat_message_t), 0);

  if (send_result < 0) {
    perror("Failed to send chat message");
  }

  free(msg);

}

// Parse and handle incoming server message 
void handle_server_message(sbcp_message_t* server_msg) {

  switch (server_msg->header.type) {

  case FWD:
  case ACK:
    // Handle forwarded or acknowledged message
    if (server_msg->attribute.type == MESSAGE) {
      printf("%s\n", server_msg->attribute.payload); 
    }
    break;

  case NAK: 
    // Handle NAK 
    if (server_msg->attribute.type == REASON) {
      printf("NAK from server: %s\n", server_msg->attribute.payload);
      exit(2);
    }
    break;
  
  // Other message types
  default:  
    break;

  }

}

// Receive and process server messages
void handle_server_messages(int server_socket) {

  sbcp_message_t* server_msg = calloc(1, sizeof(sbcp_message_t));

  if (!server_msg) {
    perror("Unable to allocate memory for server message");
    return;
  }

  int recv_result = recv(server_socket, server_msg, sizeof(sbcp_message_t), 0);

  if (recv_result <= 0) {
    // Handle receive error
    free(server_msg);
    return;
  }

  handle_server_message(server_msg);
  
  free(server_msg);

}

// Constants
#define MAX_MESSAGE_LENGTH 255
#define IDLE_TIMEOUT 10 // seconds 

// Function prototypes
void send_message(int socket, message_type_t type, const char* payload);

// Struct to hold message data
typedef struct {
  message_header_t header; 
  message_body_t body;
} message_t;

// Handle user input 
void handle_user_input(int socket) {

  char input_buffer[MAX_MESSAGE_LENGTH+1];

  // Get user input
  if(!fgets(input_buffer, sizeof(input_buffer), stdin)) {
    fprintf(stderr, "Error getting user input\n");
    return;
  }

  // Remove newline if present
  size_t length = strlen(input_buffer);
  if(input_buffer[length-1] == '\n') {
    input_buffer[length-1] = '\0'; 
  }

  // Send input message
  send_message(socket, USER_MESSAGE, input_buffer);

}

// Send SBCP message
void send_message(int socket, message_type_t type, const char* payload) {

  message_t* msg = malloc(sizeof(message_t));
  if(!msg) {
    fprintf(stderr, "Unable to allocate message\n");
    return;
  }

  // Populate header
  msg->header.version = PROTOCOL_VERSION;
  msg->header.type = type;
  msg->header.length = sizeof(message_body_t) + strlen(payload);

  // Populate body
  msg->body.type = PAYLOAD;
  msg->body.length = strlen(payload);
  strncpy(msg->body.data, payload, MAX_MESSAGE_LENGTH);

  // Send message
  int result = send(socket, msg, sizeof(message_t), 0);
  if(result < 0) {
    fprintf(stderr, "Error sending message: %s\n", strerror(errno));
  }

  free(msg);

}

// Send periodic idle message 
void* send_idle_message(void* arg) {

  int socket = *(int*)arg;

  while(1) {

    sleep(IDLE_TIMEOUT);

    if(!is_idle) { 
      send_message(socket, IDLE_MESSAGE, "");
      is_idle = true;
    }

  }

}


// Connect to server
void connect_to_server(int* socket, char* server_ip, int server_port) {

  // Create socket
  *socket = socket(AF_INET, SOCK_STREAM, 0);

  // Set up server address
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

  // Connect to server
  connect(*socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

}

// Send JOIN message 
void send_join(int socket, char* username) {
  
  message_t *msg = create_join_message(username);
  
  if(send(socket, msg, sizeof(message_t), 0) < 0) {
    perror("Error sending join");
  }

  free(msg);

}

// Handle user input
void handle_user_input(int socket) {

  char input_buffer[MAX_MESSAGE_SIZE];
  
  if(fgets(input_buffer, MAX_MESSAGE_SIZE, stdin)) {
    send_message(socket, USER_MESSAGE, input_buffer); 
  }

}

// Receive thread handler 
void* receive_handler(void* socket_ptr) {

  int socket = *(int*)socket_ptr;

  while(1) {
    handle_server_messages(socket);
  }

}

int main(int argc, char *argv[]) {

  if (argc < 4) {
    printf("Usage: %s <username> <server_ip> <server_port>\n", argv[0]);
    return 1;
  }

  char* username = argv[1];
  char* server_ip = argv[2];
  int server_port = atoi(argv[3]);

  int client_socket;

  // Validate args
  if (strlen(username) > MAX_USERNAME_LENGTH) {
    printf("Error: Username too long\n");
    return 1;
  }

  if (strlen(server_ip) > MAX_ADDRESS_LENGTH) {
    printf("Error: Invalid server IP address\n");
    return 1;
  }

  // Connect to server
  connect_to_server(&client_socket, server_ip, server_port);

  // Send join message
  send_join(client_socket, username);
  
  // Start receieve thread
  pthread_t recv_thread;
  pthread_create(&recv_thread, NULL, receive_handler, &client_socket);

  // Loop getting user input
  while (1) {
    handle_user_input(client_socket);
  }

  return 0;
}