#pragma once
#include "formats/format_cps.h"
#include "formats/format_fnt.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "game_ctx.h"
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdint.h>

static inline uint8_t VGA6To8(uint8_t v) { return (v * 255) / 63; }
#define VGA8To8(x) x

/*
    Field of vision: the 17 map positions required to read for rendering a
   screen and the 25 possible wall configurations that these positions might
   contain.

    A|B|C|D|E|F|G
      ¯ ¯ ¯ ¯ ¯
      H|I|J|K|L
        ¯ ¯ ¯
        M|N|O
        ¯ ¯ ¯
        P|^|Q
*/

typedef enum {
  A_east = 0,
  B_east,
  C_east,
  E_west,
  F_west,
  G_west,
  B_south,
  C_south,
  D_south,
  E_south,
  F_south,
  H_east,
  I_east,
  K_west,
  L_west,
  I_south,
  J_south,
  K_south,
  M_east,
  O_west,
  M_south,
  O_south,
  N_south,
  P_east,
  Q_west,
} WallRenderIndex;

void WSAFrameToPng(const uint8_t *frame, size_t size, const uint8_t *palette,
                   const char *savePngPath, int w, int h);
void CPSImageToPng(const CPSImage *image, const char *savePngPath);

void VCNImageToPng(const VCNHandle *image, const char *savePngPath);
void FNTToPng(const FNTHandle *handle, const char *savePngPath);

void SHPFrameToPng(const SHPFrame *frame, const char *savePngPath,
                   const uint8_t *palette);

void drawBackground(SDL_Renderer *renderer, const VCNHandle *vcn,
                    const VMPHandle *vmp);
void drawWall(GameContext *ctx, const VCNHandle *vcn, const VMPHandle *vmp,
              int wallType, int wallPosition);

void drawSHPFrame(SDL_Renderer *renderer, const SHPFrame *frame, int x, int y,
                  const uint8_t *palette, int scaleFactor, uint8_t xFlip);
