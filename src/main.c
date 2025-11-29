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

  int main_socket_descriptor = register_internet_socket(INADDR_ANY, PORT, 1);

  struct sockaddr_in client_socket_address = {};
  socklen_t client_socket_address_len = sizeof(client_socket_address);

  const int accepted_socket_descriptor =
      accept(main_socket_descriptor, (struct sockaddr *)&client_socket_address,
             &client_socket_address_len);

  if (accepted_socket_descriptor < 0) {
    perror("accept failed\n");
    exit(EXIT_FAILURE);
  }

  char *client_message = (char *)malloc(2048);

  if (client_message == NULL) {
    perror("malloc failed\n");
    exit(EXIT_FAILURE);
  }

  socket_message message = {client_message, 0, accepted_socket_descriptor};

  bool client_message_received = false;
  int client_message_capacity = 2048;

  while (!client_message_received) {
    int bytes_received =
        recv(message.socket_descriptor, message.contents + message.offset,
             client_message_capacity - message.offset - 1, 0);

    if (bytes_received < 0) {
      free(message.contents);
      close(message.socket_descriptor);
      close(main_socket_descriptor);
      perror("recv failed\n");
      exit(EXIT_FAILURE);
    }

    if (bytes_received == 0) {
      free(message.contents);
      close(message.socket_descriptor);
      close(main_socket_descriptor);
      printf("client closed connection\n");
      exit(0);
    }

    message.offset += bytes_received;

    if (client_message_capacity - message.offset - 1 == 0) {
      client_message_capacity *= 2;
      char *client_message_realloc =
          realloc(message.contents, client_message_capacity);

      if (client_message_realloc == NULL) {
        free(message.contents);
        close(message.socket_descriptor);
        close(main_socket_descriptor);
        printf("failed to realloc recv message buffer\n");
        exit(EXIT_FAILURE);
      }
      message.contents = client_message_realloc;
    }

    client_message_received = strstr(message.contents, "\r\n\r\n") != NULL;
  }

  message.contents[message.offset] = '\0';

  char *filename = "./src/html/index.html";
  FILE *fileptr = fopen(filename, "rb");
  if (fileptr == NULL) {
    free(message.contents);
    close(message.socket_descriptor);
    close(main_socket_descriptor);
    printf("failed to open file\n");
    exit(EXIT_FAILURE);
  }

  fseek(fileptr, 0, SEEK_END);
  long file_len = ftell(fileptr);
  fseek(fileptr, 0, SEEK_SET);

  char *file_contents = malloc(file_len);

  if (file_contents == NULL) {
    free(message.contents);
    close(message.socket_descriptor);
    close(main_socket_descriptor);
    fclose(fileptr);
    printf("failed to allocate memory for file contents\n");
    exit(EXIT_FAILURE);
  }

  int file_bytes_read = fread(file_contents, 1, file_len, fileptr);

  if (file_bytes_read < file_len) {
    free(message.contents);
    free(file_contents);
    close(message.socket_descriptor);
    close(main_socket_descriptor);
    fclose(fileptr);
    perror("fread failed\n");
    exit(EXIT_FAILURE);
  }

  char headers[512];
  int header_len = snprintf(headers, sizeof(headers),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %ld\r\n\r\n",
                            file_len);

  int total_bytes_sent = 0;

  while (total_bytes_sent < header_len) {
    int sent = send(message.socket_descriptor, headers + total_bytes_sent,
                    header_len - total_bytes_sent, 0);
    if (sent < 0) {
      free(message.contents);
      free(file_contents);
      close(message.socket_descriptor);
      close(main_socket_descriptor);
      fclose(fileptr);
      perror("send failed\n");
      exit(EXIT_FAILURE);
    }
    total_bytes_sent += sent;
  }

  total_bytes_sent = 0;

  while (total_bytes_sent < file_len) {
    int sent = send(message.socket_descriptor, file_contents + total_bytes_sent,
                    file_len - total_bytes_sent, 0);
    if (sent < 0) {
      free(message.contents);
      free(file_contents);
      close(message.socket_descriptor);
      close(main_socket_descriptor);
      fclose(fileptr);
      perror("send failed\n");
      exit(EXIT_FAILURE);
    }
    total_bytes_sent += sent;
  }

  free(message.contents);
  free(file_contents);
  close(message.socket_descriptor);
  close(main_socket_descriptor);
  fclose(fileptr);
}
