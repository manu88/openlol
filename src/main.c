
#include "format_cps.h"
#include "pak_file.h"
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t *paletteBuffer;

int main(int argc, char *argv[]) {
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(
          &file,
          "/Users/manueldeneu/Downloads/setup-00399-Lands_of_Lore-PCLinux20/"
          "sources/C/LandsOL/INTRO.PAK") == 0) {
    perror("Error while reading file");
    return 1;
  }
  printf("read %i entries\n", file.count);

  uint8_t *imgBuffer = NULL;
  for (int i = 0; i <= file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    printf("%i: Entry offset %u name '%s' ('%s') size %u \n", i, entry->offset,
           entry->filename, PakFileEntryGetExtension(entry), entry->fileSize);
    if (strcmp(entry->filename, "BACKGRND.CPS") == 0) {
      const uint8_t *buffer = PakFileGetEntryData(&file, entry);
      imgBuffer = TestCps(buffer, entry->fileSize);
      break;
    }
  }
  if (imgBuffer) {

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(320, 200, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    int maxPixVal = 0;
    for (int i = 0; i < 64000; i++) {
      uint8_t pix = imgBuffer[i];
      if (pix > maxPixVal) {
        maxPixVal = pix;
      }
      uint8_t r = (paletteBuffer[(pix * 3)] * 255) / 63;
      uint8_t g = (paletteBuffer[(pix * 3) + 1] * 255) / 63;
      uint8_t b = (paletteBuffer[(pix * 3) + 2] * 255) / 63;

      int x = i % 320;
      int y = i / 200;
      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_RenderDrawPoint(renderer, x, y);
    }

    SDL_RenderPresent(renderer);
    while (1) {
      if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
        break;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
  PAKFileRelease(&file);
  return 0;
}
