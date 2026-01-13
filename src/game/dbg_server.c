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
    DBGMsg_Header outHeader = {.type = DBGMsgType_Goodbye, 0};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
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

static void printGameState(const GameContext *gameCtx) {
  printf("itemIndexInHand %X\n", gameCtx->itemIndexInHand);
}
static int processRecvMsg(GameContext *gameCtx, const DBGMsg_Header *header,
                          uint8_t *buffer) {
  switch ((DBGMsgType)header->type) {
  case DBGMsgType_StatusRequest: {
    printf("received StatusRequest\n");
    printGameState(gameCtx);
    DBGMsg_Header outHeader = {.type = DBGMsgType_StatusResponse,
                               sizeof(DBGMsg_Status)};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
    DBGMsg_Status s = {.currentBock = gameCtx->currentBock};
    memcpy(s.gameFlags, gameCtx->engine->gameFlags, NUM_GAME_FLAGS);
    write(cltSocket, &s, sizeof(DBGMsg_Status));
  } break;
  case DBGMsgType_GiveItemRequest: {
    const DBGMSG_GiveItemRequest *req = (const DBGMSG_GiveItemRequest *)buffer;
    printf("received GiveItemRequest 0X%0X\n", req->itemId);
    DBGMsg_Header outHeader = {.type = DBGMsgType_GiveItemResponse,
                               sizeof(DBGMSG_GiveItemResponse)};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
    DBGMSG_GiveItemResponse resp;
    resp.response = GameContextAddItemToInventory(gameCtx, req->itemId);
    write(cltSocket, &resp, sizeof(DBGMSG_GiveItemResponse));
    return 1;
  };
  case DBGMsgType_QuitRequest: {
    printf("received DBGMSGQuitRequest\n");
    gameCtx->_shouldRun = 0;

    DBGMsg_Header outHeader = {.type = DBGMsgType_QuitResponse,
                               sizeof(DBGMSG_QuitResponse)};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
    DBGMSG_QuitResponse resp;
    resp.response = 1;
    write(cltSocket, &resp, sizeof(DBGMSG_QuitResponse));
    return 1;
  }
  case DBGMsgType_SetStateRequest: {
    const DBGMSG_SetStateRequest *req = (const DBGMSG_SetStateRequest *)buffer;
    printf("received DBGMSGSetStateRequest 0X%0X\n", req->state);
    GameContextSetState(gameCtx, req->state);
    DBGMsg_Header outHeader = {.type = DBGMsgType_SetStateResponse,
                               sizeof(DBGMSG_SetStateResponse)};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
    DBGMSG_SetStateResponse resp;
    resp.response = 1;
    write(cltSocket, &resp, sizeof(DBGMSG_SetStateResponse));
    return 1;
  }
  case DBGMsgType_NoClipRequest: {
    gameCtx->conf.noClip = !gameCtx->conf.noClip;
    printf("received NoClipRequest, setting no clip to %i\n",
           gameCtx->conf.noClip);
    DBGMsg_Header outHeader = {.type = DBGMsgType_NoClipResponse, 0};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
    return 1;
  }
  case DBGMsgType_SetLoggerRequest: {
    const DBGMSG_EnableLoggerRequest *req =
        (const DBGMSG_EnableLoggerRequest *)buffer;
    printf("Set logger request for prefix '%s' %i\n", req->prefix, req->enable);
    if (strcmp(req->prefix, "*") == 0) {
      if (req->enable) {
        printf("Enable std logger\n");
        LoggerSetOutput(LoggerStdOut);
      } else {
        printf("Disable std logger\n");
        LoggerSetOutput(NULL);
      }
    } else {
      if (req->enable) {
        LogEnablePrefix(req->prefix);
      } else {
        LogDisablePrefix(req->prefix);
      }
    }
    DBGMsg_Header outHeader = {.type = DBGMsgType_SetLoggerResponse, 0};
    write(cltSocket, &outHeader, sizeof(DBGMsg_Header));
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
    DBGMsg_Header header = {0};
    header.type = DBGMsgType_Hello;
    header.dataSize = 0;

    write(cltSocket, &header, sizeof(DBGMsg_Header));
  } else {

    DBGMsg_Header header = {0};
    ssize_t ret = read(cltSocket, &header, sizeof(DBGMsg_Header));
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
      assert(ret == sizeof(DBGMsg_Header));
      if (header.dataSize) {
        ssize_t r = read(cltSocket, recvBuf, header.dataSize);
        printf("read %zi bytes of data\n", r);
      }
      if (ret >= sizeof(DBGMsg_Header)) {
        printf("received msg %i %i\n", header.type, header.dataSize);
        return processRecvMsg(gameCtx, &header,
                              header.dataSize ? recvBuf : NULL);
      }
    }
  }
  return 0;
}
