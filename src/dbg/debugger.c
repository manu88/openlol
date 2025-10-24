#include "dbg_msgs.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static char cmdInputBuffer[1024];
static uint8_t recvBuf[1024];
static int shouldStop = 0;
static int sock = 0;

static void processCommand(int argc, char *argv[]) {
  const char *cmd = argv[0];
  if (strcmp(cmd, "exit") == 0) {
    shouldStop = 1;
  } else if (strcmp(cmd, "give") == 0) {
    if (argc < 2) {
      printf("missing itemID\n");
      return;
    }

    DBGMsg_Header header = {.type = DBGMsgType_GiveItemRequest,
                            sizeof(DBGMSG_GiveItemRequest)};
    write(sock, &header, sizeof(DBGMsg_Header));
    DBGMSG_GiveItemRequest req;
    req.itemId = atoi(argv[1]);
    write(sock, &req, sizeof(DBGMSG_GiveItemRequest));

    read(sock, &header, sizeof(DBGMsg_Header));
    assert(header.type == DBGMsgType_GiveItemResponse);
    DBGMSG_GiveItemResponse resp;
    read(sock, &resp, sizeof(DBGMSG_GiveItemResponse));
    printf("reply %i\n", resp.response);
  } else if (strcmp(cmd, "status") == 0) {
    DBGMsg_Header header = {.type = DBGMsgType_StatusRequest, 0};
    write(sock, &header, sizeof(DBGMsg_Header));
    read(sock, &header, sizeof(DBGMsg_Header));
    DBGMsg_Status status;
    read(sock, &status, sizeof(DBGMsg_Status));
    printf("received %i %i current block %X\n", header.type, header.dataSize,
           status.currentBock);
  } else if ((strcmp(cmd, "state") == 0) && argc > 1) {
    DBGMsg_Header header = {.type = DBGMsgType_SetStateRequest,
                            sizeof(DBGMSG_SetStateRequest)};
    write(sock, &header, sizeof(DBGMsg_Header));

    DBGMSG_SetStateRequest req;
    req.state = atoi(argv[1]);
    write(sock, &req, sizeof(DBGMSG_SetStateRequest));
    read(sock, &header, sizeof(DBGMsg_Header));
    assert(header.type == DBGMsgType_SetStateResponse);
    DBGMSG_SetStateResponse resp;
    read(sock, &resp, sizeof(DBGMSG_SetStateResponse));
    printf("reply %i\n", resp.response);
  } else if (strcmp(cmd, "quit") == 0) {
    DBGMsg_Header header = {.type = DBGMsgType_QuitRequest, 0};
    write(sock, &header, sizeof(DBGMsg_Header));

    read(sock, &header, sizeof(DBGMsg_Header));
    assert(header.type == DBGMsgType_QuitResponse);
    DBGMSG_QuitResponse resp;
    read(sock, &resp, sizeof(DBGMSG_QuitResponse));
    printf("reply %i\n", resp.response);
    shouldStop = 1;
  } else if (strcmp(cmd, "noclip") == 0) {
    DBGMsg_Header header = {.type = DBGMsgType_NoClipRequest, 0};
    write(sock, &header, sizeof(DBGMsg_Header));
    read(sock, &header, sizeof(DBGMsg_Header));
    assert(header.type == DBGMsgType_NoClipResponse);
  } else if (strcmp(cmd, "log") == 0 && argc > 2) {
    DBGMsg_Header header = {.type = DBGMsgType_SetLoggerRequest,
                            sizeof(DBGMSG_EnableLoggerRequest)};
    write(sock, &header, sizeof(DBGMsg_Header));
    DBGMSG_EnableLoggerRequest req = {0};
    strncpy(req.prefix, argv[1], sizeof(req.prefix));
    req.enable = atoi(argv[2]);
    write(sock, &req, sizeof(DBGMSG_EnableLoggerRequest));
    read(sock, &header, sizeof(DBGMsg_Header));
    assert(header.type == DBGMsgType_SetLoggerResponse);

  } else {
    printf("unknown command '%s'\n", cmd);
  }
}

static void readCommand(const char *cmd) {
  printf(">: ");
  fflush(stdout);
  if (cmd) {
    printf("%s\n", cmd);
    strncpy(cmdInputBuffer, cmd, strlen(cmd));
    cmdInputBuffer[strlen(cmd)] = 0;
  } else {
    ssize_t ret = read(STDIN_FILENO, cmdInputBuffer, sizeof(cmdInputBuffer));
    if (ret <= 0) {
      perror("read input");
    }
    cmdInputBuffer[ret - 1] = 0;
  }

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
    printf("Invalid address/ Address not supported\n");
    return 1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("Connection Failed\n");
    return 1;
  }
  return 0;
}

static void processRecvMsg(const DBGMsg_Header *header, uint8_t *buffer) {
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

const char defaultIP[] = "127.0.0.1";

int cmdDbg(int argc, char *argv[]) {
  const char *host = defaultIP;

  if (argc > 0 && strcmp(argv[0], "--") != 0) {
    host = argv[0];
    argc -= 1;
    argv += 1;
  }

  if (argc > 0 && strcmp(argv[0], "--") == 0) {
    argc -= 1;
    argv += 1;
  }
  printf("connect to %s:%i\n", host, DBG_PORT);

  if (connectToServer(host) != 0) {
    return 1;
  }

  DBGMsg_Header header = {0};
  assert(read(sock, &header, sizeof(DBGMsg_Header)) == sizeof(DBGMsg_Header));

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
  for (int i = 0; i < argc; i++) {
    readCommand(argv[i]);
  }
  while (!shouldStop) {
    readCommand(NULL);
  }

  close(sock);
  return 0;
}
