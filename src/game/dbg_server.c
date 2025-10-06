#include "dbg_server.h"
#include "dbg_msgs.h"
#include "game_ctx.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int server_fd;
static struct sockaddr_in address;
int cltSocket = -1;

void DBGServerRelease(void) {
  printf("DBGServerRelease");
  if (cltSocket != -1) {
    DBGMsgHeader outHeader = {.type = DBGMsgType_Goodbye, sizeof(DBGMsgStatus)};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
  }
  close(cltSocket);
}

int DBGServerInit(void) {
  printf("INIT DBG Server\n");

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("In socket");
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(DBG_PORT);

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

static void processRecvMsg(const GameContext *gameCtx,
                           const DBGMsgHeader *header, uint8_t *buffer) {
  switch ((DBGMsgType)header->type) {

  case DBGMsgType_StatusRequest: {

    printf("received StatusRequest\n");
    DBGMsgHeader outHeader = {.type = DBGMsgType_StatusResponse,
                              sizeof(DBGMsgStatus)};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
    DBGMsgStatus s = {.currentBock = gameCtx->currentBock};
    write(cltSocket, &s, sizeof(DBGMsgStatus));
  } break;
  case DBGMsgType_GiveItemRequest: {
    const DBGMSGGiveItemRequest *req = (const DBGMSGGiveItemRequest *)buffer;
    printf("received GiveItemRequest 0X%0X\n", req->itemId);
    DBGMsgHeader outHeader = {.type = DBGMsgType_GiveItemResponse,
                              sizeof(DBGMSGGiveItemResponse)};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
    DBGMSGGiveItemResponse resp;
    resp.response = 44;
    write(cltSocket, &resp, sizeof(DBGMSGGiveItemResponse));
  } break;
  case DBGMsgType_StatusResponse:
  case DBGMsgType_Hello:
  default:
    assert(0);
  }
}

static uint8_t recvBuf[1024];

void DBGServerUpdate(const GameContext *gameCtx) {
  int addrlen = sizeof(address);

  if (cltSocket == -1) {
    if ((cltSocket = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t *)&addrlen)) < 0) {
      if (errno == EAGAIN) {
        return;
      }
      perror("In accept");
      return;
    }
    printf("new client\n");
    fcntl(cltSocket, F_SETFL, O_NONBLOCK);
    DBGMsgHeader header = {0};
    header.type = DBGMsgType_Hello;
    header.dataSize = 0;

    write(cltSocket, &header, sizeof(DBGMsgHeader));
  } else {
    DBGMsgHeader header = {0};
    ssize_t ret = read(cltSocket, &header, sizeof(DBGMsgHeader));
    if (ret == 0) {
      printf("Client connection closed\n");
      close(cltSocket);
      cltSocket = -1;
    } else if (ret == -1) {
      if (errno == EAGAIN) {
        return;
      } else {
        perror("read");
      }
    } else {
      printf("received %zi bytes of header\n", ret);
      assert(ret == sizeof(DBGMsgHeader));
      if (header.dataSize) {
        ssize_t r = read(cltSocket, recvBuf, header.dataSize);
        printf("read %zi bytes of data\n", r);
      }
      if (ret >= sizeof(DBGMsgHeader)) {
        printf("received msg %i %i\n", header.type, header.dataSize);
        processRecvMsg(gameCtx, &header, header.dataSize ? recvBuf : NULL);
      }
    }
  }
}
