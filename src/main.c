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
  char contents[MAX_MESSAGE_LENGTH];
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

  socket_message message = {"", 0, accepted_socket_descriptor};

  bool message_received = false;
  int bytes_read = 0;

  while (!message_received) {
    bytes_read +=
        recv(accepted_socket_descriptor, &message.contents + bytes_read,
             sizeof(message.contents) - bytes_read, 0);

    message_received = strstr(&message.contents[0], "\r\n\r\n") != NULL;
  }

  printf("buffer contents: %s", message.contents);

  char *filename = "./src/html/index.html";
  FILE *fileptr = fopen(filename, "r");
  fseek(fileptr, 0, SEEK_END);
  long file_bytes = ftell(fileptr);
  fseek(fileptr, 0, SEEK_SET);

  char file_contents[file_bytes];

  fread(file_contents, 1, file_bytes, fileptr);

  char mes[1024];

  snprintf(mes, sizeof(mes),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: %ld\r\n"
           "\r\n"
           "%s",
           file_bytes, file_contents);

  int bytes_sent = send(accepted_socket_descriptor, mes, sizeof(mes), 0);
  printf("bytes sent: %d\n", bytes_sent);
  close(socket_descriptor);
  close(accepted_socket_descriptor);
  fclose(fileptr);
}
