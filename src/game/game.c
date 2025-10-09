#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_mouse.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "bytes.h"
#include "dbg_server.h"
#include "formats/format_cps.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_callbacks.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "geometry.h"
#include "render.h"
#include "renderer.h"
#include "script.h"
#include "script_builtins.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int getChapterId(int levelId) {
  assert(levelId <= 5);
  return 1;
}

static SAVHandle savHandle = {0};

static int GameRun(GameContext *gameCtx);
static void usageGame(void) { printf("game [savefile.dat]\n"); }

int loadSaveFile(const char *filepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    return 0;
  }
  return SAVHandleFromBuffer(&savHandle, buffer, readSize);
}

int cmdGame(int argc, char *argv[]) {
  if (argc > 0 && strcmp(argv[0], "-h") == 0) {
    usageGame();
    return 1;
  }
  char *savFile = NULL;

  if (argc > 0) {
    savFile = argv[0];
  }

  if (savFile) {
    printf("Loading sav file '%s'\n", savFile);
    if (loadSaveFile(savFile) == 0) {
      printf("Error while reading file '%s'\n", savFile);
      return 1;
    }
  } else {
    printf("New game\n");
    SAVHandleGetNewGame(&savHandle);
  }

  assert(GameEnvironmentInit("data"));
  int levelId = savHandle.slot.general->currentLevel;
  int chapterId = getChapterId(levelId);
  assert(GameEnvironmentLoadChapter(chapterId));

  GameContext gameCtx = {0};
  if (!GameContextInit(&gameCtx)) {
    return 1;
  }

  GameContextInstallCallbacks(&gameCtx.interp);
  gameCtx.interp.callbackCtx = &gameCtx;

  LevelContext levelCtx = {0};
  gameCtx.level = &levelCtx;

  GameContextLoadLevel(&gameCtx, levelId, savHandle.slot.general->currentBlock,
                       savHandle.slot.general->currentDirection);

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    uint16_t gameObjIndex = savHandle.slot.inventory[i];
    if (gameObjIndex == 0) {
      continue;
    }
    const GameObject *obj = savHandle.slot.gameObjects + gameObjIndex;
    printf("%i: 0X%X 0X%X\n", i, gameObjIndex, obj->itemId);
    gameCtx.inventory[i] = obj->itemId;
  }

  // FACE01.SHP
  char faceFile[11] = "";
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    uint8_t charId = savHandle.slot.characters[i]->id > 0
                         ? savHandle.slot.characters[i]->id
                         : -savHandle.slot.characters[i]->id;
    memcpy(&gameCtx.chars[i], savHandle.slot.characters[i],
           sizeof(SAVCharacter));
    if (charId == 0) {
      continue;
    }
    printf("character %i=%i\n", i, charId);
    snprintf(faceFile, 11, "FACE%02i.SHP", charId);
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, faceFile));
    SHPHandleFromCompressedBuffer(&gameCtx.charFaces[i], f.buffer,
                                  f.bufferSize);
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "INVENT1.CPS"));
    assert(CPSImageFromBuffer(&gameCtx.inventoryBackground, f.buffer,
                              f.bufferSize));
  }

  createCursorForItem(&gameCtx, 0);

  GameRun(&gameCtx);
  LevelContextRelease(&levelCtx);
  GameContextRelease(&gameCtx);

  printf("GameEnvironmentRelease\n");
  GameEnvironmentRelease();
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  MazeHandleRelease(&levelCtx->mazHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static void clickOnFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);

  GameContextRunScript(gameCtx, nextBlock);
}

static void moveFront(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->y -= 1;
    break;
  case East:
    pt->x += 1;
    break;
  case South:
    pt->y += 1;
    break;
  case West:
    pt->x -= 1;
    break;
  }
}

static void moveBack(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->y += 1;
    break;
  case East:
    pt->x -= 1;
    break;
  case South:
    pt->y -= 1;
    break;
  case West:
    pt->x += 1;
    break;
  }
}

static void moveLeft(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->x -= 1;
    break;
  case East:
    pt->y -= 1;
    break;
  case South:
    pt->x += 1;
    break;
  case West:
    pt->y += 1;
    break;
  }
}

static void moveRight(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->x += 1;
    break;
  case East:
    pt->y += 1;
    break;
  case South:
    pt->x -= 1;
    break;
  case West:
    pt->y -= 1;
    break;
  }
}
static Orientation turnLeft(Orientation orientation) {
  orientation -= 1;
  if ((int)orientation < 0) {
    return West;
  }
  return orientation;
}

static Orientation turnRight(Orientation orientation) {
  orientation += 1;
  if (orientation > West) {
    return North;
  }
  return orientation;
}

static int charPortraitClicked(const GameContext *gameCtx) {
  if (gameCtx->mouseEv.pos.y < CHAR_FACE_Y ||
      gameCtx->mouseEv.pos.y > CHAR_FACE_Y + CHAR_FACE_H) {
    return -1;
  }

  uint8_t numChars = GameContextGetNumChars(gameCtx);
  switch (numChars) {
  case 1:
    if (gameCtx->mouseEv.pos.x > CHAR_FACE_0_1_X &&
        gameCtx->mouseEv.pos.x < CHAR_FACE_0_1_X + CHAR_FACE_W) {
      return 0;
    }
    break;
  case 2: {
    if (gameCtx->mouseEv.pos.x > CHAR_FACE_0_2_X &&
        gameCtx->mouseEv.pos.x < CHAR_FACE_0_2_X + CHAR_FACE_W) {
      return 0;
    } else if (gameCtx->mouseEv.pos.x > CHAR_FACE_1_2_X &&
               gameCtx->mouseEv.pos.x < CHAR_FACE_1_2_X + CHAR_FACE_W) {
      return 1;
    }
  } break;
  case 3:
  default:
    assert(0);
  }

  return -1;
}

static int mouseIsInInventoryStrip(GameContext *gameCtx) {
  return gameCtx->mouseEv.pos.x >= UI_INVENTORY_BUTTON_X &&
         gameCtx->mouseEv.pos.y >= UI_INVENTORY_BUTTON_Y &&
         gameCtx->mouseEv.pos.x <
             (UI_INVENTORY_BUTTON_X + (UI_MENU_INV_BUTTON_W * 11)) &&
         gameCtx->mouseEv.pos.y <
             (UI_INVENTORY_BUTTON_Y + (UI_MENU_INV_BUTTON_W * 1));
}

static int processInventoryStripMouse(GameContext *gameCtx) {
  int x = gameCtx->mouseEv.pos.x - UI_INVENTORY_BUTTON_X;
  int buttonX = (int)(x / UI_MENU_INV_BUTTON_W);
  // 0 is left arrow, 10 is right arrow
  if (buttonX == 0) {
    int inventoryIndex =
        gameCtx->inventoryIndex - (gameCtx->mouseEv.isRightClick ? 9 : 1);
    if (inventoryIndex < 0) {
      inventoryIndex = INVENTORY_SIZE - 1;
    }
    gameCtx->inventoryIndex = inventoryIndex;
    return 1;
  } else if (buttonX == 10) {
    int inventoryIndex =
        gameCtx->inventoryIndex + (gameCtx->mouseEv.isRightClick ? 9 : 1);
    if (inventoryIndex >= INVENTORY_SIZE) {
      inventoryIndex = 0;
    }
    gameCtx->inventoryIndex = inventoryIndex;
    return 1;
  } else {
    printf("Inventory button %i\n", buttonX);
  }
  return 0;
}

static int processCharInventoryMouse(GameContext *gameCtx) {
  if (gameCtx->mouseEv.pos.x >= INVENTORY_SCREEN_EXIT_BUTTON_X &&
      gameCtx->mouseEv.pos.y >= INVENTORY_SCREEN_EXIT_BUTTON_Y &&
      gameCtx->mouseEv.pos.x <
          INVENTORY_SCREEN_EXIT_BUTTON_X + INVENTORY_SCREEN_EXIT_BUTTON_W &&
      gameCtx->mouseEv.pos.y <
          INVENTORY_SCREEN_EXIT_BUTTON_Y + INVENTORY_SCREEN_EXIT_BUTTON_H) {
    printf("exit inventory\n");
    GameContextSetState(gameCtx, GameState_PlayGame);
    return 1;
  } else if (mouseIsInInventoryStrip(gameCtx)) {
    return processInventoryStripMouse(gameCtx);
  } else {

    int charIndex = charPortraitClicked(gameCtx);
    if (charIndex != -1) {
      printf("Char %i %i\n", charIndex, gameCtx->chars[charIndex].id);
      GameContextSetState(gameCtx, GameState_ShowInventory);
      return 1;
    } else {
      printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
    }
  }
  return 0;
}

static int processPlayGameMouse(GameContext *gameCtx) {
  if (gameCtx->mouseEv.pos.x >= UI_TURN_LEFT_BUTTON_X &&
      gameCtx->mouseEv.pos.y >= UI_TURN_LEFT_BUTTON_Y &&
      gameCtx->mouseEv.pos.x <
          (UI_TURN_LEFT_BUTTON_X + (UI_DIR_BUTTON_W * 3)) &&
      gameCtx->mouseEv.pos.y <
          (UI_TURN_LEFT_BUTTON_Y + (UI_DIR_BUTTON_W * 2))) {
    int x = gameCtx->mouseEv.pos.x - UI_TURN_LEFT_BUTTON_X;
    int y = gameCtx->mouseEv.pos.y - UI_TURN_LEFT_BUTTON_Y;
    int buttonX = (int)(x / UI_DIR_BUTTON_W);
    int buttonY = (int)(y / UI_DIR_BUTTON_W);
    if (buttonX <= 2 && buttonY <= 1) {
      if (buttonY == 0) {
        if (buttonX == 0) {
          gameCtx->orientation = turnLeft(gameCtx->orientation);
        } else if (buttonX == 1) {
          moveFront(&gameCtx->partyPos, gameCtx->orientation);
        } else if (buttonX == 2) {
          gameCtx->orientation = turnRight(gameCtx->orientation);
        }
      } else if (buttonY == 1) {
        if (buttonX == 0) {
          moveLeft(&gameCtx->partyPos, gameCtx->orientation);
        } else if (buttonX == 1) {
          moveBack(&gameCtx->partyPos, gameCtx->orientation);
        } else if (buttonX == 2) {
          moveRight(&gameCtx->partyPos, gameCtx->orientation);
        }
      }
      return 1;
    }
  } else if (gameCtx->mouseEv.pos.x >= UI_MENU_BUTTON_X &&
             gameCtx->mouseEv.pos.y >= UI_MENU_BUTTON_Y &&
             gameCtx->mouseEv.pos.x <
                 (UI_MENU_BUTTON_X + (UI_MENU_INV_BUTTON_W * 2)) &&
             gameCtx->mouseEv.pos.y <
                 (UI_MENU_BUTTON_Y + (UI_MENU_INV_BUTTON_H * 1))) {
    int x = gameCtx->mouseEv.pos.x - UI_MENU_BUTTON_X;
    int buttonX = (int)(x / UI_MENU_INV_BUTTON_W);
    if (buttonX == 0) {
      printf("Menu button\n");
    } else if (buttonX == 1) {
      printf("sleep button\n");
    }
  } else if (mouseIsInInventoryStrip(gameCtx)) {
    return processInventoryStripMouse(gameCtx);
  } else if (gameCtx->mouseEv.pos.x >= MAZE_COORDS_X &&
             gameCtx->mouseEv.pos.y >= MAZE_COORDS_Y &&
             (gameCtx->mouseEv.pos.x < (MAZE_COORDS_X + MAZE_COORDS_W)) &&
             (gameCtx->mouseEv.pos.y < (MAZE_COORDS_Y + MAZE_COORDS_H))) {
    int x = gameCtx->mouseEv.pos.x - MAZE_COORDS_X;
    int y = gameCtx->mouseEv.pos.y - MAZE_COORDS_Y;
    printf("maze click %i %i\n", x, y);
    clickOnFrontWall(gameCtx);
    return 1;
  } else {
    int charIndex = charPortraitClicked(gameCtx);
    if (charIndex != -1) {
      printf("Char %i %i\n", charIndex, gameCtx->chars[charIndex].id);
      GameContextSetState(gameCtx, GameState_ShowInventory);
      return 1;
    } else {
      printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
    }
  }
  return 0;
}

static int processMouse(GameContext *gameCtx) {
  switch (gameCtx->state) {
  case GameState_PlayGame:
    return processPlayGameMouse(gameCtx);
  case GameState_ShowInventory:
    return processCharInventoryMouse(gameCtx);
  case GameState_TimAnimation:
    return 0;
    break;
  }
  return 0;
}

static int processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
  int shouldUpdate = 0;
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    assert(gameCtx->mouseEv.pending == 0); // prev one needs to be handled
    gameCtx->mouseEv.pending = 1;
    gameCtx->mouseEv.pos.x = e->motion.x / SCREEN_FACTOR;
    gameCtx->mouseEv.pos.y = e->motion.y / SCREEN_FACTOR;
    gameCtx->mouseEv.isRightClick = e->button.button == 3;

  } else if (e->type == SDL_KEYDOWN) {
    switch (e->key.keysym.sym) {
    case SDLK_z:
      // go front
      shouldUpdate = 1;
      moveFront(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_s:
      // go back
      shouldUpdate = 1;
      moveBack(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_q:
      // go left
      shouldUpdate = 1;
      moveLeft(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_d:
      // go right
      shouldUpdate = 1;
      moveRight(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_a:
      // turn anti-clockwise
      shouldUpdate = 1;
      gameCtx->orientation = turnLeft(gameCtx->orientation);

      break;
    case SDLK_e:
      // turn clockwise
      shouldUpdate = 1;
      gameCtx->orientation = turnRight(gameCtx->orientation);
      break;
    case SDLK_SPACE:
      shouldUpdate = 1;
      clickOnFrontWall(gameCtx);
      break;
    default:
      break;
    }
  }
  return shouldUpdate;
}

static void animateDialogZone(GameContext *gameCtx) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(gameCtx->pixBuf, &rect, &data, &pitch);

  // copy outline
  int offset = gameCtx->dialogBoxFrames;
  const int size = 20;
  for (int y = DIALOG_BOX_H - size; y < DIALOG_BOX_H; y++) {
    const uint32_t *rowSource = (unsigned int *)((char *)data + pitch * y);
    uint32_t *rowDest = (unsigned int *)((char *)data + pitch * (y + offset));
    for (int x = 0; x < DIALOG_BOX_W; x++) {
      rowDest[x] = rowSource[x];
    }
  }

  if (gameCtx->dialogBoxFrames >= size) {
    // need to copy the dialog background
    int size = DIALOG_BOX_H - 1;
    if (gameCtx->dialogBoxFrames == DIALOG_BOX_H2 - DIALOG_BOX_H) {
      size++;
    }
    for (int y = 1; y < size; y++) {
      const uint32_t *rowSource = (unsigned int *)((char *)data + pitch * y);
      uint32_t *rowDest =
          (unsigned int *)((char *)data + pitch * (y + (DIALOG_BOX_H - 2)));
      for (int x = 0; x < DIALOG_BOX_W; x++) {
        rowDest[x] = rowSource[x];
      }
    }
  }

  if (gameCtx->dialogBoxFrames <= (DIALOG_BOX_H2 - DIALOG_BOX_H)) {
    gameCtx->dialogBoxFrames++;
  }
  SDL_UnlockTexture(gameCtx->pixBuf);
}

static void renderInventorySlot(GameContext *gameCtx, uint8_t slot,
                                uint16_t frameId) {
  assert(slot <= 9);
  SHPFrame frame = {0};
  SHPHandleGetFrame(&gameCtx->itemShapes, &frame, frameId);
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame,
               UI_INVENTORY_BUTTON_X + (UI_MENU_INV_BUTTON_W * (1 + slot)) + 2,
               UI_INVENTORY_BUTTON_Y, gameCtx->defaultPalette);
}

static uint16_t getItemSHPFrameIndex(uint16_t itemId) {
  switch (itemId) {
  case 0XD9:
    return 43;
  case 0XDA:
    return 42;
  case 0XD8:
    return 30;
  case 0X2C:
    return 7;
  }
  printf("getItemSHPFrameIndex: unhandled %X\n", itemId);
  assert(0);
  return 0;
}

static void renderCharFace(GameContext *gameCtx, uint8_t charId, int x) {
  SHPFrame frame = {0};
  assert(SHPHandleGetFrame(&gameCtx->charFaces[charId], &frame, 0));
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame, x, CHAR_FACE_Y,
               gameCtx->defaultPalette);
}

static void renderCharFaces(GameContext *gameCtx) {
  uint8_t numChars = GameContextGetNumChars(gameCtx);
  switch (numChars) {
  case 1:
    renderCharFace(gameCtx, 0, CHAR_FACE_0_1_X);
    break;
  case 2:
    renderCharFace(gameCtx, 0, CHAR_FACE_0_2_X);
    renderCharFace(gameCtx, 1, CHAR_FACE_1_2_X);
    break;
  case 3:
  default:
    assert(0);
  }
}

static void renderCharInventory(GameContext *gameCtx) {
  renderCPSAt(gameCtx->pixBuf, gameCtx->inventoryBackground.data,
              gameCtx->inventoryBackground.imageSize,
              gameCtx->inventoryBackground.palette, INVENTORY_SCREEN_X,
              INVENTORY_SCREEN_Y, INVENTORY_SCREEN_W, INVENTORY_SCREEN_H);
}

static void renderInventory(GameContext *gameCtx) {
  for (int i = 0; i < 9; i++) {
    uint16_t index = (gameCtx->inventoryIndex + i) % INVENTORY_SIZE;
    uint16_t itemId = gameCtx->inventory[index];
    if (itemId) {
      renderInventorySlot(gameCtx, i, getItemSHPFrameIndex(itemId));
    }
  }
}
static void GameRender(GameContext *gameCtx) {
  SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gameCtx->renderer);
  renderPlayField(gameCtx);

  if (gameCtx->fadeOutFrames) {
    gameCtx->fadeOutFrames--;
    clearMazeZone(gameCtx);
  } else if (gameCtx->state == GameState_ShowInventory) {
    renderCharInventory(gameCtx);
  } else {
    GameRenderScene(gameCtx);
    if (gameCtx->state == GameState_TimAnimation) {
      if (GameTimAnimatorRender(&gameCtx->timAnimator) == 0) {
        GameContextSetState(gameCtx, GameState_PlayGame);
      }
    }
  }

  renderInventory(gameCtx);

  renderCharFaces(gameCtx);

  if (gameCtx->showBigDialog) {
    animateDialogZone(gameCtx);
  }

  renderDialog(gameCtx);
  // GameRenderMap(gameCtx, 640, 350);

  SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                   PIX_BUF_HEIGHT * SCREEN_FACTOR};
  assert(SDL_RenderCopy(gameCtx->renderer, gameCtx->pixBuf, NULL, &dest) == 0);
  SDL_RenderPresent(gameCtx->renderer);
}

static int GameRun(GameContext *gameCtx) {
  int quit = 0;
  int shouldUpdate = 1;
  int firstTime = 1;
  // Event loop
  while (!quit) {
    if (DBGServerUpdate(gameCtx)) {
      shouldUpdate = 1;
    }
    while (gameCtx->state == GameState_PlayGame &&
           EMCInterpreterIsValid(&gameCtx->interp, &gameCtx->interpState)) {
      EMCInterpreterRun(&gameCtx->interp, &gameCtx->interpState);
      shouldUpdate = 1;
    }

    SDL_Event e;
    SDL_WaitEventTimeout(&e, 20);
    if (e.type == SDL_QUIT) {
      quit = 1;
    }
    if (gameCtx->state == GameState_PlayGame ||
        gameCtx->state == GameState_ShowInventory) {
      if (processGameInputs(gameCtx, &e)) {
        shouldUpdate = 1;
      }
      if (shouldUpdate) {
        uint16_t gameX = 0;
        uint16_t gameY = 0;
        GetGameCoords(gameCtx->partyPos.x, gameCtx->partyPos.y, &gameX, &gameY);
        gameCtx->currentBock = BlockFromCoords(gameX, gameY);
      }
    } else if (gameCtx->state == GameState_TimAnimation) {
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
        shouldUpdate = 1;
      }
    }
    if (firstTime) {
      shouldUpdate = 1;
      firstTime = 0;
    }
    if (gameCtx->mouseEv.pending) {
      if (processMouse(gameCtx)) {
        shouldUpdate = 1;
      }
      gameCtx->mouseEv.pending = 0;
    }
    if (shouldUpdate) {
      GameRender(gameCtx);
      shouldUpdate = 0;
    }
  }

  SDL_Quit();
  return 0;
}
