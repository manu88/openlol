#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_rect.h"
#include "SDL_render.h"
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

static SAVHandle savHandle = {0};
static int GameRun(GameContext *gameCtx);
static void usageGame(void) { printf("game [-d datadir] [savefile.dat]\n"); }

static int loadSaveFile(const char *filepath) {

  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    return 0;
  }
  if (!SAVHandleFromBuffer(&savHandle, buffer, readSize)) {
    return 0;
  }

  return 1;
}

static GameContext gameCtx = {0};
int cmdGame(int argc, char *argv[]) {
  const char *dataDir = NULL;
  optind = 0;
  char c;
  while ((c = getopt(argc, argv, "hd:")) != -1) {
    switch (c) {
    case 'h':
      usageGame();
      return 0;
    case 'd':
      dataDir = optarg;
      break;
    }
  }

  argc = argc - optind;
  argv = argv + optind;

  char *savFile = NULL;

  if (argc > 0) {
    savFile = argv[0];
  }

  assert(GameEnvironmentInit(dataDir ? dataDir : "data"));

  GameEnvironmentLoadPak("O01A.PAK");
  GameEnvironmentLoadPak("O01E.PAK");
  GameEnvironmentLoadPak("O01D.PAK");

  if (!GameContextInit(&gameCtx)) {
    return 1;
  }

  GameContextInstallCallbacks(&gameCtx.interp);
  gameCtx.interp.callbackCtx = &gameCtx;

  GameContextStartup(&gameCtx);
  LevelContext levelCtx = {0};
  gameCtx.level = &levelCtx;

  if (savFile) {
    printf("Loading sav file '%s'\n", savFile);
    if (loadSaveFile(savFile) == 0) {
      printf("Error while reading file '%s'\n", savFile);
      return 1;
    }

    gameCtx.levelId = savHandle.slot.general->currentLevel;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
      uint16_t gameObjIndex = savHandle.slot.inventory[i];
      if (gameObjIndex == 0) {
        continue;
      }
      const GameObject *obj = savHandle.slot.gameObjects + gameObjIndex;
      gameCtx.inventory[i] = obj->itemId;
    }

    for (int i = 0; i < NUM_CHARACTERS; i++) {
      memcpy(&gameCtx.chars[i], savHandle.slot.characters[i],
             sizeof(SAVCharacter));
    }
    gameCtx.currentBock = savHandle.slot.general->currentBlock;
    gameCtx.orientation = savHandle.slot.general->currentDirection;
  } else {
    gameCtx.levelId = 1;
    gameCtx.chars[0].id = -9; // Ak'shel for the win
    snprintf(gameCtx.chars[0].name, 11, "Ak'shel");
    // temp until we get the value from script/tim
    gameCtx.currentBock = 0X24D;
    gameCtx.orientation = North;
    gameCtx.inventory[0] = 216;
    gameCtx.inventory[1] = 217;
    gameCtx.inventory[2] = 218;
  }

  char faceFile[11] = "";
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    uint8_t charId =
        gameCtx.chars[i].id > 0 ? gameCtx.chars[i].id : -gameCtx.chars[i].id;
    if (charId == 0) {
      continue;
    }
    snprintf(faceFile, 11, "FACE%02i.SHP", charId);
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, faceFile));
    SHPHandleFromCompressedBuffer(&gameCtx.charFaces[i], f.buffer,
                                  f.bufferSize);
  }

  GameContextLoadLevel(&gameCtx, gameCtx.levelId);

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

static void selectFromInventoryStrip(GameContext *gameCtx, int index) {
  int realIndex = (gameCtx->inventoryIndex + index) % INVENTORY_SIZE;
  uint16_t itemId = gameCtx->inventory[realIndex];
  if (gameCtx->itemInHand == 0 && itemId) {
    gameCtx->inventory[realIndex] = 0;
  } else {
    gameCtx->inventory[realIndex] = gameCtx->itemInHand;
  }
  gameCtx->itemInHand = itemId;
  createCursorForItem(
      gameCtx, itemId ? GameContextGetItemSHPFrameIndex(gameCtx, itemId) : 0);
  if (gameCtx->itemInHand == 0) {
    return;
  }
  uint16_t stringId = gameCtx->itemProperties[gameCtx->itemInHand].stringId;

  GameContextGetString(gameCtx, stringId, gameCtx->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  gameCtx->dialogText = gameCtx->dialogTextBuffer;
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
    selectFromInventoryStrip(gameCtx, buttonX - 1);
    return 1;
  }
  return 0;
}

static int processAnimationMouse(GameContext *gameCtx) {
  if (gameCtx->mouseEv.pos.y >= DIALOG_BUTTON_Y_2 &&
      gameCtx->mouseEv.pos.y < DIALOG_BUTTON_Y_2 + DIALOG_BUTTON_H) {
    if (gameCtx->mouseEv.pos.x >= DIALOG_BUTTON1_X &&
        gameCtx->mouseEv.pos.x < DIALOG_BUTTON1_X + DIALOG_BUTTON_W) {
      TIMInterpreterButtonClicked(&gameCtx->timAnimator.timInterpreter, 0);
      printf("button 0\n");
    } else if (gameCtx->mouseEv.pos.x >= DIALOG_BUTTON2_X &&
               gameCtx->mouseEv.pos.x < DIALOG_BUTTON2_X + DIALOG_BUTTON_W) {
      printf("button 1\n");
      TIMInterpreterButtonClicked(&gameCtx->timAnimator.timInterpreter, 1);
    } else if (gameCtx->mouseEv.pos.x >= DIALOG_BUTTON3_X &&
               gameCtx->mouseEv.pos.x < DIALOG_BUTTON3_X + DIALOG_BUTTON_W) {
      printf("button 2\n");
      TIMInterpreterButtonClicked(&gameCtx->timAnimator.timInterpreter, 2);
    }
  }
  // printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
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
      gameCtx->selectedChar = charIndex;
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
          gameCtx->orientation = OrientationTurnLeft(gameCtx->orientation);
        } else if (buttonX == 1) {
          gameCtx->partyPos =
              PointGoFront(&gameCtx->partyPos, gameCtx->orientation, 1);
        } else if (buttonX == 2) {
          gameCtx->orientation = OrientationTurnRight(gameCtx->orientation);
        }
      } else if (buttonY == 1) {
        if (buttonX == 0) {
          gameCtx->partyPos =
              PointGoLeft(&gameCtx->partyPos, gameCtx->orientation, 1);
        } else if (buttonX == 1) {
          gameCtx->partyPos =
              PointGoBack(&gameCtx->partyPos, gameCtx->orientation, 1);
        } else if (buttonX == 2) {
          gameCtx->partyPos =
              PointGoRight(&gameCtx->partyPos, gameCtx->orientation, 1);
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
      gameCtx->selectedChar = charIndex;
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
    return processAnimationMouse(gameCtx);
    break;
  case GameState_GrowDialogBox:
  case GameState_ShrinkDialogBox:
  case GameState_Invalid:
    assert(0);
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
  } else if (!gameCtx->controlDisabled && e->type == SDL_KEYDOWN) {
    switch (e->key.keysym.sym) {
    case SDLK_z:
      // go front
      shouldUpdate = 1;
      gameCtx->partyPos =
          PointGoFront(&gameCtx->partyPos, gameCtx->orientation, 1);
      break;
    case SDLK_s:
      // go back
      shouldUpdate = 1;
      gameCtx->partyPos =
          PointGoBack(&gameCtx->partyPos, gameCtx->orientation, 1);
      break;
    case SDLK_q:
      // go left
      shouldUpdate = 1;
      gameCtx->partyPos =
          PointGoLeft(&gameCtx->partyPos, gameCtx->orientation, 1);
      break;
    case SDLK_d:
      // go right
      shouldUpdate = 1;
      gameCtx->partyPos =
          PointGoRight(&gameCtx->partyPos, gameCtx->orientation, 1);
      break;
    case SDLK_a:
      // turn anti-clockwise
      shouldUpdate = 1;
      gameCtx->orientation = OrientationTurnLeft(gameCtx->orientation);

      break;
    case SDLK_e:
      // turn clockwise
      shouldUpdate = 1;
      gameCtx->orientation = OrientationTurnRight(gameCtx->orientation);
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

static int GamePreUpdate(GameContext *gameCtx) {
  int shouldUpdate = 0;
  while (gameCtx->state == GameState_PlayGame &&
         EMCInterpreterIsValid(&gameCtx->interp, &gameCtx->interpState)) {
    EMCInterpreterRun(&gameCtx->interp, &gameCtx->interpState);
    shouldUpdate = 1;
  }
  return shouldUpdate;
}

static void GameRunOnce(GameContext *gameCtx) {
  int shouldUpdate = 0;
  if (DBGServerUpdate(gameCtx)) {
    shouldUpdate = 1;
  }

  if (GamePreUpdate(gameCtx)) {
    shouldUpdate = 1;
  }

  SDL_Event e;
  SDL_WaitEventTimeout(&e, 20);
  if (e.type == SDL_QUIT) {
    gameCtx->_shouldRun = 0;
  }

  switch (gameCtx->state) {
  case GameState_PlayGame:
  case GameState_ShowInventory:
    if (processGameInputs(gameCtx, &e)) {
      shouldUpdate = 1;
    }
    if (shouldUpdate) {
      uint16_t gameX = 0;
      uint16_t gameY = 0;
      GetGameCoords(gameCtx->partyPos.x, gameCtx->partyPos.y, &gameX, &gameY);
      gameCtx->currentBock = BlockFromCoords(gameX, gameY);
    }
    break;
  case GameState_TimAnimation:
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
      shouldUpdate = 1;
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
      assert(gameCtx->mouseEv.pending == 0); // prev one needs to be handled
      gameCtx->mouseEv.pending = 1;
      gameCtx->mouseEv.pos.x = e.motion.x / SCREEN_FACTOR;
      gameCtx->mouseEv.pos.y = e.motion.y / SCREEN_FACTOR;
      gameCtx->mouseEv.isRightClick = e.button.button == 3;
    }
    break;
  case GameState_GrowDialogBox:
  case GameState_ShrinkDialogBox:
    shouldUpdate = 1;
    break;
  case GameState_Invalid:
    assert(0);
    break;
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

  SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                   PIX_BUF_HEIGHT * SCREEN_FACTOR};
  assert(SDL_RenderCopy(gameCtx->renderer, gameCtx->backgroundPixBuf, NULL,
                        &dest) == 0);
  if (gameCtx->state != GameState_ShowInventory) {

    SDL_Rect source = {MAZE_COORDS_X, MAZE_COORDS_Y, MAZE_COORDS_W,
                       MAZE_COORDS_H};
    dest.x = MAZE_COORDS_X * SCREEN_FACTOR;
    dest.y = MAZE_COORDS_Y * SCREEN_FACTOR;
    dest.w = MAZE_COORDS_W * SCREEN_FACTOR;
    dest.h = MAZE_COORDS_H * SCREEN_FACTOR;
    assert(SDL_RenderCopy(gameCtx->renderer, gameCtx->foregroundPixBuf, &source,
                          &dest) == 0);
  }
  SDL_RenderPresent(gameCtx->renderer);
}

static int GameRun(GameContext *gameCtx) {
  gameCtx->_shouldRun = 1;
  while (gameCtx->_shouldRun) {
    GameRunOnce(gameCtx);
  }
  SDL_Quit();
  return 0;
}
