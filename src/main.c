#include <sys/socket.h>

int main() { const int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0); }
