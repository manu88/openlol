#include "dbg_server.h"
#include "dbg_msgs.h"
#include "game_ctx.h"
#include "logger.h"
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

int recvDataSize;
static uint8_t recvBuf[1024];

void DBGServerRelease(void) {
  printf("DBGServerRelease\n");
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

static int processRecvMsg(GameContext *gameCtx, const DBGMsgHeader *header,
                          uint8_t *buffer) {
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
    resp.response = GameContextAddItemToInventory(gameCtx, req->itemId);
    write(cltSocket, &resp, sizeof(DBGMSGGiveItemResponse));
    return 1;
  };
  case DBGMsgType_QuitRequest: {
    printf("received DBGMSGQuitRequest\n");
    gameCtx->_shouldRun = 0;

    DBGMsgHeader outHeader = {.type = DBGMsgType_QuitResponse,
                              sizeof(DBGMSGQuitResponse)};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
    DBGMSGQuitResponse resp;
    resp.response = 1;
    write(cltSocket, &resp, sizeof(DBGMSGQuitResponse));
    return 1;
  }
  case DBGMsgType_SetStateRequest: {
    const DBGMSGSetStateRequest *req = (const DBGMSGSetStateRequest *)buffer;
    printf("received DBGMSGSetStateRequest 0X%0X\n", req->state);
    GameContextSetState(gameCtx, req->state);
    DBGMsgHeader outHeader = {.type = DBGMsgType_SetStateResponse,
                              sizeof(DBGMSGSetStateResponse)};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
    DBGMSGSetStateResponse resp;
    resp.response = 1;
    write(cltSocket, &resp, sizeof(DBGMSGSetStateResponse));
    return 1;
  }
  case DBGMsgType_NoClipRequest: {
    gameCtx->_noClip = !gameCtx->_noClip;
    printf("received NoClipRequest, setting no clip to %i\n", gameCtx->_noClip);
    DBGMsgHeader outHeader = {.type = DBGMsgType_NoClipResponse, 0};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
    return 1;
  }
  case DBGMsgType_SetLoggerRequest: {

    if (LoggerGetOutput() == NULL) {
      printf("Enable std logger\n");
      LoggerSetOutput(LoggerStdOut);
    } else {
      printf("Disable std logger\n");
      LoggerSetOutput(NULL);
    }
    DBGMsgHeader outHeader = {.type = DBGMsgType_SetLoggerResponse, 0};
    write(cltSocket, &outHeader, sizeof(DBGMsgHeader));
    return 1;
  }
  case DBGMsgType_NoClipResponse:
  case DBGMsgType_SetStateResponse:
  case DBGMsgType_GiveItemResponse:
  case DBGMsgType_StatusResponse:
  case DBGMsgType_SetLoggerResponse:
  case DBGMsgType_Hello:
  default:
    assert(0);
  }
  return 0;
}

int DBGServerUpdate(GameContext *gameCtx) {
  int addrlen = sizeof(address);

  if (cltSocket == -1) {
    if ((cltSocket = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t *)&addrlen)) < 0) {
      if (errno == EAGAIN) {
        return 0;
      }
      perror("In accept");
      return 0;
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
        return 0;
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
        return processRecvMsg(gameCtx, &header,
                              header.dataSize ? recvBuf : NULL);
      }
    }
  }
  return 0;
}
