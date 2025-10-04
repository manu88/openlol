#include "dbg_server.h"
#include "dbg_msgs.h"
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
int cltSocket;

void DBGServerRelease(void) { close(cltSocket); }
int DBGServerInit(void) {
  printf("INIT DBG Server\n");

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("In socket");
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  memset(address.sin_zero, '\0', sizeof address.sin_zero);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("In bind");
    return 1;
  }
  if (listen(server_fd, 10) < 0) {
    perror("In listen");
    return 1;
  }

  fcntl(server_fd, F_SETFL, O_NONBLOCK);
  return 0;
}

void DBGServerUpdate(void) {
  int addrlen = sizeof(address);

  if ((cltSocket = accept(server_fd, (struct sockaddr *)&address,
                          (socklen_t *)&addrlen)) < 0) {
    if (errno == EAGAIN) {
      return;
    }
    perror("In accept");
    return;
  }

  DBGMsgHeader header = {0};
  header.type = DBGMsgType_Hello;
  header.dataSize = 12;

  write(cltSocket, &header, sizeof(DBGMsgHeader));
}
