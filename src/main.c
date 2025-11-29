#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define DEFAULT_ALLOCATION_SIZE 2048

typedef struct socket_message {
  char *contents;
  int contents_len;
  int offset;
  int socket_descriptor;
} socket_message;

typedef struct http_request {
  char *headers;
  int headers_len;
  char *body;
  int body_len;
  char *path;
  int path_len;
  char method[6];
} http_request;

int register_main_socket(int ip, int port, int max_queue_len,
                         int *main_socket) {

  const int main_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);

  if (main_socket_descriptor < 0) {
    perror("socket failed\n");
    return -1;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(ip);
  address.sin_port = htons(port);

  socklen_t socket_address_len = sizeof(address);

  const int bind_result = bind(main_socket_descriptor,
                               (struct sockaddr *)&address, socket_address_len);

  if (bind_result < 0) {
    perror("bind failed\n");
    close(main_socket_descriptor);
    return -1;
  }

  const int listen_result = listen(main_socket_descriptor, max_queue_len);
  if (listen_result < 0) {
    perror("listen failed\n");
    close(main_socket_descriptor);
    return -1;
  }

  *main_socket = main_socket_descriptor;

  return 0;
}

int get_request_socket(int main_socket_descriptor, socket_message *message) {

  struct sockaddr_in client_socket_address = {};
  socklen_t client_socket_address_len = sizeof(client_socket_address);

  const int accepted_socket_descriptor =
      accept(main_socket_descriptor, (struct sockaddr *)&client_socket_address,
             &client_socket_address_len);

  if (accepted_socket_descriptor < 0) {
    perror("accept failed\n");
    return -1;
  }

  char *client_message = (char *)malloc(DEFAULT_ALLOCATION_SIZE);

  if (client_message == NULL) {
    perror("malloc failed\n");
    close(accepted_socket_descriptor);
    return -1;
  }

  message->socket_descriptor = accepted_socket_descriptor;
  message->contents = client_message;
  message->contents_len = DEFAULT_ALLOCATION_SIZE;
  message->offset = 0;

  return 0;
}

int get_http_request(socket_message *message) {

  bool client_message_received = false;

  while (!client_message_received) {
    int bytes_received =
        recv(message->socket_descriptor, message->contents + message->offset,
             message->contents_len - message->offset - 1, 0);

    if (bytes_received < 0) {
      perror("recv failed\n");
      return -1;
    }

    if (bytes_received == 0) {
      printf("client closed connection\n");
      return -1;
    }

    message->offset += bytes_received;

    if (message->contents_len - message->offset - 1 == 0) {
      message->contents_len *= 2;
      char *client_message_realloc =
          realloc(message->contents, message->contents_len);

      if (client_message_realloc == NULL) {
        printf("failed to realloc recv message buffer\n");
        return -1;
      }
      message->contents = client_message_realloc;
    }

    client_message_received = strstr(message->contents, "\r\n\r\n") != NULL;
  }

  message->contents[message->offset] = '\0';

  return 0;
}

int read_file(char *file_path, char **file_contents, long *file_length) {

  FILE *file_ptr = fopen(file_path, "rb");
  if (file_ptr == NULL) {
    printf("failed to open file\n");
    return -1;
  }

  fseek(file_ptr, 0, SEEK_END);
  *file_length = ftell(file_ptr);
  fseek(file_ptr, 0, SEEK_SET);

  *file_contents = malloc(*file_length);

  if (*file_contents == NULL) {
    printf("failed to allocate memory for file contents\n");
    fclose(file_ptr);
    return -1;
  }

  int file_bytes_read = fread(*file_contents, 1, *file_length, file_ptr);

  if (file_bytes_read < *file_length) {
    perror("fread failed\n");
    free(*file_contents);
    fclose(file_ptr);
    return -1;
  }

  fclose(file_ptr);
  return 0;
}

int send_response(socket_message message, char *response,
                  long response_length) {

  char headers[512];
  int header_len = snprintf(headers, sizeof(headers),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %ld\r\n\r\n",
                            response_length);

  int total_bytes_sent = 0;

  while (total_bytes_sent < header_len) {
    int sent = send(message.socket_descriptor, headers + total_bytes_sent,
                    header_len - total_bytes_sent, 0);
    if (sent < 0) {
      perror("send failed\n");
      return -1;
    }
    total_bytes_sent += sent;
  }

  total_bytes_sent = 0;

  while (total_bytes_sent < response_length) {
    int sent = send(message.socket_descriptor, response + total_bytes_sent,
                    response_length - total_bytes_sent, 0);
    if (sent < 0) {
      perror("send failed\n");
      return -1;
    }
    total_bytes_sent += sent;
  }
  return 0;
}

int parse_http_request(http_request *http_request, socket_message message) {
  http_request->body = NULL;
  http_request->body_len = 0;

  char *ptr = strchr(message.contents, ' ');
  if (ptr == NULL) {
    perror("unable to parse http method\n");
    return -1;
  }

  int method_len = ptr - message.contents;
  strncpy(http_request->method, message.contents, method_len);
  http_request->method[method_len] = '\0';
  http_request->headers = message.contents;
  http_request->headers_len = message.contents_len;

  char path_base[] = "./dist/html/";
  char *path_start = ptr + 2;
  char *path_end = strchr(path_start, ' ');

  if (path_end == NULL) {
    perror("unable to parse http path\n");
    return -1;
  }

  int path_len = path_end - path_start;
  int base_len = strlen(path_base);
  int total_len = path_len + base_len;

  http_request->path = malloc(total_len + 1);

  if (http_request->path == NULL) {
    perror("unable to malloc for path\n");
    return -1;
  }

  strncpy(http_request->path, path_base, base_len);
  strncpy(http_request->path + base_len, path_start, path_len);
  http_request->path[total_len] = '\0';
  http_request->path_len = total_len;

  printf("path is:\n");
  printf("%s\n", http_request->path);
  char *found = strstr(http_request->path, "..");
  if (found != NULL) {
    perror("bad path provided by user\n");
    free(http_request->path);
    return -1;
  }

  return 0;
}

int main() {

  int main_socket;
  int result = register_main_socket(INADDR_ANY, PORT, 1, &main_socket);

  if (result < 0) {
    exit(EXIT_FAILURE);
  }

  socket_message message = {};
  result = get_request_socket(main_socket, &message);

  if (result < 0) {
    close(main_socket);
    exit(EXIT_FAILURE);
  }

  result = get_http_request(&message);

  if (result < 0) {
    close(main_socket);
    close(message.socket_descriptor);
    free(message.contents);
    exit(EXIT_FAILURE);
  }

  http_request request = {};
  result = parse_http_request(&request, message);
  if (result < 0) {
    close(main_socket);
    free(message.contents);
    close(message.socket_descriptor);
    exit(EXIT_FAILURE);
  }

  char *file_contents = NULL;
  long file_length = 0;

  printf("%s", request.path);
  result = read_file(request.path, &file_contents, &file_length);

  if (result < 0) {
    close(main_socket);
    free(message.contents);
    close(message.socket_descriptor);
    free(request.path);
    exit(EXIT_FAILURE);
  }

  result = send_response(message, file_contents, file_length);

  if (result < 0) {
    close(main_socket);
    close(message.socket_descriptor);
    free(message.contents);
    free(file_contents);
    free(request.path);
    exit(EXIT_FAILURE);
  }

  close(main_socket);
  close(message.socket_descriptor);
  free(message.contents);
  free(file_contents);
  free(request.path);
}
