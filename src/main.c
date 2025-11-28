#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_MESSAGE_LENGTH 1024

typedef struct socket_message {
  char *contents;
  int offset;
  int socket_descriptor;
} socket_message;

int register_internet_socket(int ip, int port, int max_queue_len) {

  const int socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);

  if (socket_descriptor < 0) {
    perror("socket failed\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(ip);
  address.sin_port = htons(port);

  socklen_t socket_address_len = sizeof(address);

  const int bind_result =
      bind(socket_descriptor, (struct sockaddr *)&address, socket_address_len);

  if (bind_result < 0) {
    perror("bind failed\n");
    exit(EXIT_FAILURE);
  }

  const int listen_result = listen(socket_descriptor, max_queue_len);
  if (listen_result < 0) {
    perror("listen failed\n");
    exit(EXIT_FAILURE);
  }
  return socket_descriptor;
}

int main() {

  int socket_descriptor = register_internet_socket(INADDR_ANY, PORT, 1);

  struct sockaddr_in client_socket_address = {};
  socklen_t client_socket_address_len = sizeof(client_socket_address);

  const int accepted_socket_descriptor =
      accept(socket_descriptor, (struct sockaddr *)&client_socket_address,
             &client_socket_address_len);

  if (accepted_socket_descriptor < 0) {
    perror("accept failed\n");
    exit(EXIT_FAILURE);
  }

  char *recv_message = (char *)malloc(2048);
  socket_message message = {recv_message, 0, accepted_socket_descriptor};

  bool message_received = false;
  int bytes_read = 0;
  int recv_message_capacity = 2048;

  while (!message_received) {
    int recv_read =
        recv(accepted_socket_descriptor, message.contents + bytes_read,
             recv_message_capacity - bytes_read, 0);

    bytes_read += recv_read;

    if (recv_read < 0) {
      perror("recv failed\n");
      exit(EXIT_FAILURE);
    }

    if (recv_read == 0) {
      printf("client closed connection\n");
      exit(0);
    }
    if (bytes_read != 0 && recv_message_capacity - bytes_read == 0) {

      recv_message_capacity *= 2;
      recv_message = (char *)realloc(recv_message, recv_message_capacity);
      message.contents = recv_message;
    }

    message_received = strstr(message.contents, "\r\n\r\n") != NULL;
  }

  char *filename = "./src/html/index.html";
  FILE *fileptr = fopen(filename, "rb");
  fseek(fileptr, 0, SEEK_END);
  long file_bytes = ftell(fileptr);
  fseek(fileptr, 0, SEEK_SET);

  char *file_contents = malloc(file_bytes + 1);

  int file_bytes_read = fread(file_contents, 1, file_bytes, fileptr);
  file_contents[file_bytes] = '\0';

  if (file_bytes_read < file_bytes) {
    perror("accept failed\n");
    exit(EXIT_FAILURE);
  }

  char mes[1024];

  snprintf(mes, sizeof(mes),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: %ld\r\n"
           "\r\n"
           "%s",
           file_bytes, file_contents);

  int bytes_sent = send(accepted_socket_descriptor, mes, strlen(mes), 0);
  printf("bytes sent: %d\n", bytes_sent);
  close(socket_descriptor);
  close(accepted_socket_descriptor);
  fclose(fileptr);
}
