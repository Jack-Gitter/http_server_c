#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_MESSAGE_LENGTH 1024

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

  char buffer[MAX_MESSAGE_LENGTH];
  recv(accepted_socket_descriptor, &buffer, sizeof(buffer), 0);

  printf("buffer contents: %s", buffer);
  close(socket_descriptor);
  close(accepted_socket_descriptor);
}
