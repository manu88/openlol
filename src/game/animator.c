#include "animator.h"
#include "renderer.h"
#include <assert.h>
#include <string.h>

void AnimatorInit(Animator *animator) { memset(animator, 0, sizeof(Animator)); }

void AnimatorRelease(Animator *animator) {
  if (animator->wsaFrameBuffer) {
    free(animator->wsaFrameBuffer);
  }
}

void AnimatorInitWSA(Animator *animator, const uint8_t *buffer,
                     size_t bufferSize, int x, int y, int offscreen,
                     int flags) {
  WSAHandleFromBuffer(&animator->wsa, buffer, bufferSize);
  if (animator->wsa.header.palette == NULL) {
    printf("WSA has no palette, using the game level one\n");
    animator->wsa.header.palette = animator->defaultPalette;
  }
  animator->wsaX = x;
  animator->wsaY = y;
  animator->wsaFlags = flags;
  if (animator->wsaFrameBuffer) {
    free(animator->wsaFrameBuffer);
  }
  animator->wsaFrameBuffer =
      malloc(animator->wsa.header.width * animator->wsa.header.height);
  memset(animator->wsaFrameBuffer, 0,
         animator->wsa.header.width * animator->wsa.header.height);
  assert(animator->wsaFrameBuffer);
}

void AnimatorRenderWSAFrame(Animator *animator) {

  const uint8_t *imgData = animator->wsaFrameBuffer;
  size_t dataSize = animator->wsa.header.width * animator->wsa.header.height;
  const uint8_t *paletteBuffer = animator->wsa.header.palette;

  assert(animator->pixBuf);
  void *data;
  int pitch;
  SDL_LockTexture(animator->pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < animator->wsa.header.width; x++) {
    for (int y = 0; y < animator->wsa.header.height; y++) {
      int offset = (animator->wsa.header.width * y) + x;
      if (offset >= dataSize) {
        printf("Offset %i >= %zu\n", offset, dataSize);
      }
      assert(offset < dataSize);
      uint8_t paletteIdx = *(imgData + offset);

      uint8_t r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
      uint8_t g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
      uint8_t b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);
      if (r == 0 && g == 0 && b == 0) {
        continue;
      }
      uint32_t *row =
          (unsigned int *)((char *)data + pitch * (animator->wsaY + y));
      // if (r && g && b) {
      row[animator->wsaX + x] = 0XFF000000 + (r << 0X10) + (g << 0X8) + b;
      //}
    }
  }
  SDL_UnlockTexture(animator->pixBuf);
}

void AnimatorSetupPart(Animator *animator, uint16_t animIndex, uint16_t part,
                       uint16_t firstFrame, uint16_t lastFrame, uint16_t cycles,
                       uint16_t nextPart, uint16_t partDelay, uint16_t field,
                       uint16_t sfxIndex, uint16_t sfxFrame) {}

void AnimatorPlayPart(Animator *animator, uint16_t animIndex,
                      uint16_t firstFrame, uint16_t lastFrame, uint16_t delay) {
}
