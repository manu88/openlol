#pragma once
#include "format_cps.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include <SDL2/SDL.h>

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

void CPSImageToPng(const CPSImage *image, const char *savePngPath);

void VCNImageToPng(const VCNHandle *image, const char *savePngPath);

void testRenderScene(const VCNHandle *vcn, const VMPHandle *vmp, int wallPos);

void RenderScene(SDL_Renderer *renderer, const VCNHandle *vcn,
                 const VMPHandle *vmp, int wallType);

void drawBackground(SDL_Renderer *renderer, const VCNHandle *vcn,
                    const VMPHandle *vmp);
void drawWall(SDL_Renderer *renderer, const VCNHandle *vcn,
              const VMPHandle *vmp, int wallType, int wallPosition);
