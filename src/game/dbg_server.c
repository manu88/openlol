#include "dbg_server.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

static int server_fd;
static struct sockaddr_in address;

int DBGServerInit(void) {
  printf("INIT DBG Server\n");

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("In socket");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  memset(address.sin_zero, '\0', sizeof address.sin_zero);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("In bind");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 10) < 0) {
    perror("In listen");
    exit(EXIT_FAILURE);
  }

  fcntl(server_fd, F_SETFL, O_NONBLOCK);
  return 0;
}

static char buffer[1024] = {0};

void DBGServerUpdate(void) {
  int new_socket;
  int addrlen = sizeof(address);
  long valread;

  char *hello = "Hello from server";
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t *)&addrlen)) < 0) {
    if (errno == EAGAIN) {
      return;
    }
    perror("In accept");
    return;
  }

  valread = read(new_socket, buffer, 1024);
  printf("%s\n", buffer);
  write(new_socket, hello, strlen(hello));
  printf("------------------Hello message sent-------------------\n");
  close(new_socket);
}
