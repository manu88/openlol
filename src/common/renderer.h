#pragma once
#include "format_cps.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include <SDL2/SDL.h>

void CPSImageToPng(const CPSImage *image, const char *savePngPath);

void VCNImageToPng(const VCNHandle *image, const char *savePngPath);

void testRenderScene(const VCNHandle *vcn, const VMPHandle *vmp, int wallPos);

void RenderScene(SDL_Renderer *renderer, const VCNHandle *vcn,
                 const VMPHandle *vmp, int wallType);

void drawBackground(SDL_Renderer *renderer, const VCNHandle *vcn,
                    const VMPHandle *vmp);
void drawWall(SDL_Renderer *renderer, const VCNHandle *vcn,
              const VMPHandle *vmp, int wallType, int wallPosition);
