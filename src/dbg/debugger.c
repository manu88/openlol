#include "dbg_msgs.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_ssize_t.h>
#include <sys/socket.h>
#include <unistd.h>

static char cmdInputBuffer[1024];

static int shouldStop = 0;
static int sock = 0;

static void processCommand(int argc, char *argv[]) {
  const char *cmd = argv[0];
  if (strcmp(cmd, "exit") == 0) {
    shouldStop = 1;
  } else if (strcmp(cmd, "give") == 0) {
    if (argc < 2) {
      printf("missing itemID");
      return;
    }

    DBGMsgHeader header = {.type = DBGMsgType_GiveItemRequest,
                           sizeof(DBGMSGGiveItemRequest)};
    write(sock, &header, sizeof(DBGMsgHeader));
    printf("sent header\n");
    DBGMSGGiveItemRequest req;
    req.itemId = atoi(argv[1]);
    write(sock, &req, sizeof(DBGMSGGiveItemRequest));
    printf("sent data\n");

    read(sock, &header, sizeof(DBGMsgHeader));
    printf("Got reply\n");
    assert(header.type == DBGMsgType_GiveItemResponse);
    DBGMSGGiveItemResponse resp;
    read(sock, &resp, sizeof(DBGMSGGiveItemResponse));
    printf("reply %i\n", resp.response);
  } else if (strcmp(cmd, "status") == 0) {
    DBGMsgHeader header = {.type = DBGMsgType_StatusRequest, 0};
    write(sock, &header, sizeof(DBGMsgHeader));

    read(sock, &header, sizeof(DBGMsgHeader));
    DBGMsgStatus status;
    read(sock, &status, sizeof(DBGMsgStatus));
    printf("received %i %i current block %X\n", header.type, header.dataSize,
           status.currentBock);
  } else {
    printf("unknow command '%s'\n", cmd);
  }
}

static void readCommand(void) {
  printf(">: ");
  fflush(stdout);
  ssize_t ret = read(STDIN_FILENO, cmdInputBuffer, sizeof(cmdInputBuffer));
  if (ret <= 0) {
    perror("read input");
  }
  cmdInputBuffer[ret - 1] = 0;
  char *str = (char *)cmdInputBuffer;
  char *tok = NULL;
  int argc = 0;
  char **argv = NULL;
  while ((tok = strsep(&str, " ")) != NULL) {
    argv = realloc(argv, (argc + 1) * sizeof(char *));
    argv[argc] = tok;
    argc++;
  }
  if (argc) {
    processCommand(argc, argv);
  }
  free(argv);
}

static int connectToServer(const char *ip) {

  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return 1;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(DBG_PORT);

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

static uint8_t recvBuf[1024];

static void processRecvMsg(const DBGMsgHeader *header, uint8_t *buffer) {
  switch ((DBGMsgType)header->type) {

  case DBGMsgType_Hello:
    printf("received Hello\n");
    break;

  case DBGMsgType_StatusResponse:
    break;

  case DBGMsgType_StatusRequest:
  default:
    assert(0);
  }
}

int cmdDbg(int argc, char *argv[]) {
  printf("Start debugger\n");

  if (connectToServer("127.0.0.1") != 0) {
    return 1;
  }

  DBGMsgHeader header = {0};
  assert(read(sock, &header, sizeof(DBGMsgHeader)) == sizeof(DBGMsgHeader));

  printf("received type=%i size=%i\n", header.type, header.dataSize);
  if (header.dataSize) {
    ssize_t dataSizeRead = read(sock, recvBuf, sizeof(recvBuf));
    if (dataSizeRead <= 0) {
      perror("read data");
      return 0;
    }
    processRecvMsg(&header, header.dataSize ? recvBuf : NULL);
  }

  shouldStop = 0;
  while (!shouldStop) {
    readCommand();
  }

  close(sock);

  return 0;
}
