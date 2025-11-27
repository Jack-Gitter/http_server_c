#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  const int socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);

  if (socket_descriptor < 0) {
    perror("socket failed\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(3000);

  socklen_t socket_address_len = sizeof(address);

  const int bind_result =
      bind(socket_descriptor, (struct sockaddr *)&address, socket_address_len);

  if (bind_result < 0) {
    perror("bind failed\n");
    exit(EXIT_FAILURE);
  }

  const int listen_result = listen(socket_descriptor, 1);
  if (listen_result < 0) {
    perror("listen failed\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in client_socket_address = {};
  socklen_t client_socket_address_len = sizeof(client_socket_address);

  const int accepted_socket_descriptor =
      accept(socket_descriptor, (struct sockaddr *)&client_socket_address,
             &client_socket_address_len);

  if (accepted_socket_descriptor < 0) {
    perror("accept failed\n");
    exit(EXIT_FAILURE);
  }

  char buffer[1024];
  recv(accepted_socket_descriptor, &buffer, sizeof(buffer), 0);

  printf("buffer contents: %s", buffer);
  close(socket_descriptor);
  close(accepted_socket_descriptor);
}
