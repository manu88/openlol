#include "dbg_msgs.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

static char cmdInputBuffer[1024];

static int shouldStop = 0;

static void readCommand(void) {
  printf(">: ");
  fflush(stdout);
  ssize_t ret = read(STDIN_FILENO, cmdInputBuffer, sizeof(cmdInputBuffer));
  if (ret <= 0) {
    perror("read input");
  }
  cmdInputBuffer[ret - 1] = 0;
  if (strcmp(cmdInputBuffer, "exit") == 0) {
    shouldStop = 1;
  }
  printf("'%s'\n", cmdInputBuffer);
}

static int sock = 0;

static int connectToServer(const char *ip) {

  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return 1;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return 1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return 1;
  }
  return 0;
}

int cmdDbg(int argc, char *argv[]) {
  printf("Start debugger\n");

  if (connectToServer("127.0.0.1") != 0) {
    return 1;
  }

  DBGMsgHeader header = {0};

  assert(read(sock, &header, sizeof(DBGMsgHeader)) == sizeof(DBGMsgHeader));

  printf("received type=%i size=%i\n", header.type, header.dataSize);

  shouldStop = 0;
  while (!shouldStop) {
    readCommand();
  }

  close(sock);

  return 0;
}
