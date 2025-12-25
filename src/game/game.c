#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "dbg_server.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_wll.h"
#include "game_callbacks.h"
#include "game_char_inventory.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "geometry.h"
#include "logger.h"
#include "menu.h"
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int GameRun(GameContext *gameCtx);

static void usageGame(void) {
  printf("game [-d datadir] [-l langId] [-a] [savefile-or-savedir]\n");
}

static int pathIsFile(const char *path) {
  struct stat path_stat;
  if (stat(path, &path_stat) < 0) {
    return 1; // later read of the file will fail
  }
  return S_ISREG(path_stat.st_mode);
}

static GameContext gameCtx = {0};

int cmdGame(int argc, char *argv[]) {
  const char *dataDir = NULL;
  Language lang = Language_EN;
  optind = 0;
  char c;
  int doLogs = 0;
  while ((c = getopt(argc, argv, "ahd:l:")) != -1) {
    switch (c) {
    case 'h':
      usageGame();
      return 0;
    case 'a':
      doLogs = 1;
      break;
    case 'd':
      dataDir = optarg;
      break;
    case 'l':
      lang = atoi(optarg);
      break;
    }
  }
  if (doLogs) {
    LoggerSetOutput(LoggerStdOut);
  }

  printf("using lang %s\n", LanguageGetExtension(lang));
  argc = argc - optind;
  argv = argv + optind;

  char *savFileOrDir = NULL;

  if (argc > 0) {
    savFileOrDir = argv[0];
  }

  assert(GameEnvironmentInit(dataDir ? dataDir : "data", lang));

  if (!GameContextInit(&gameCtx, lang)) {
    return 1;
  }

  GameContextInstallCallbacks(&gameCtx.interp);
  gameCtx.interp.callbackCtx = &gameCtx;

  GameContextStartup(&gameCtx);
  LevelContext levelCtx = {0};
  gameCtx.level = &levelCtx;

  if (savFileOrDir && pathIsFile(savFileOrDir)) {
    printf("Loading sav file '%s'\n", savFileOrDir);
    if (GameContextLoadSaveFile(&gameCtx, savFileOrDir) == 0) {
      printf("Error while reading file '%s'\n", savFileOrDir);
      return 1;
    }
    GameContextSetState(&gameCtx, GameState_PlayGame);
  }
  GameContextSetSavDir(&gameCtx, savFileOrDir);

  GameContextUpdateCursor(&gameCtx);

  GameRun(&gameCtx);
  LevelContextRelease(&levelCtx);

  ConfigHandleWriteFile(&gameCtx.conf, "conf.txt");
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

static void inspectFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);
  const MazeBlock *block =
      gameCtx->level->mazHandle.maze->wallMappingIndices + nextBlock;
  uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  if (mapping) {
    printf("clickOnFrontWall WallType wallType=0X%X decorationId=%X "
           "specialType=%X "
           "flags=%X automapData=%X\n",
           mapping->wallType, mapping->decorationId, mapping->specialType,
           mapping->flags, mapping->automapData);
  }
}

static void runBlockScript(GameContext *gameCtx) {
  GameContextRunScript(gameCtx, gameCtx->currentBock);
}

static void clickOnFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);
  const MazeBlock *block =
      gameCtx->level->mazHandle.maze->wallMappingIndices + nextBlock;
  uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  if (!mapping) {
    return;
  }
  switch (mapping->specialType) {
  case WallSpecialType_None:
    printf("WallSpecialType_None\n");
    break;
  case WallSpecialType_WallShape:
    printf("WallSpecialType_WallShape\n");
    break;
  case WallSpecialType_LeverOn:
    printf("WallSpecialType_LeverOn\n");
    break;
  case WallSpecialType_LeverOff:
    printf("WallSpecialType_LeverOff\n");
    break;
  case WallSpecialType_OnlyScript:
    printf("WallSpecialType_OnlyScript\n");
    break;
  case WallSpecialType_DoorSwitch:
    printf("WallSpecialType_DoorSwitch\n");
    break;
  case WallSpecialType_Niche:
    printf("WallSpecialType_Niche\n");
    break;
  default:
    assert(0);
  }
  GameContextRunScript(gameCtx, nextBlock);
}

static int charZoneClicked(const GameContext *gameCtx) {
  if (gameCtx->mouseEv.pos.y < CHAR_ZONE_Y ||
      gameCtx->mouseEv.pos.y > CHAR_ZONE_Y + CHAR_ZONE_H) {
    return -1;
  }

  uint8_t numChars = GameContextGetNumChars(gameCtx);
  switch (numChars) {
  case 1:
    if (gameCtx->mouseEv.pos.x > CHAR_ZONE_0_1_X &&
        gameCtx->mouseEv.pos.x < CHAR_ZONE_0_1_X + CHAR_ZONE_W) {
      return 0;
    }
    break;
  case 2: {
    if (gameCtx->mouseEv.pos.x > CHAR_ZONE_0_2_X &&
        gameCtx->mouseEv.pos.x < CHAR_ZONE_0_2_X + CHAR_ZONE_W) {
      return 0;
    } else if (gameCtx->mouseEv.pos.x > CHAR_ZONE_1_2_X &&
               gameCtx->mouseEv.pos.x < CHAR_ZONE_1_2_X + CHAR_ZONE_W) {
      return 1;
    }
  } break;
  case 3: {
    if (gameCtx->mouseEv.pos.x > CHAR_ZONE_0_3_X &&
        gameCtx->mouseEv.pos.x < CHAR_ZONE_0_3_X + CHAR_ZONE_W) {
      return 0;
    } else if (gameCtx->mouseEv.pos.x > CHAR_ZONE_1_3_X &&
               gameCtx->mouseEv.pos.x < CHAR_ZONE_1_3_X + CHAR_ZONE_W) {
      return 1;
    } else if (gameCtx->mouseEv.pos.x > CHAR_ZONE_2_3_X &&
               gameCtx->mouseEv.pos.x < CHAR_ZONE_2_3_X + CHAR_ZONE_W) {
      return 2;
    }
  } break;
  default:
    assert(0);
  }

  return -1;
}

static void selectFromInventoryStrip(GameContext *gameCtx, int index) {
  int realIndex = (gameCtx->inventoryIndex + index) % INVENTORY_SIZE;
  uint16_t itemIndex = gameCtx->inventory[realIndex];
  if (gameCtx->itemIndexInHand == 0 && itemIndex) {
    gameCtx->inventory[realIndex] = 0;
  } else {
    gameCtx->inventory[realIndex] = gameCtx->itemIndexInHand;
  }

  gameCtx->itemIndexInHand = itemIndex;
  GameContextUpdateCursor(gameCtx);
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
      printf("button 0\n");
      if (gameCtx->dialogState == DialogState_InProgress) {
        gameCtx->dialogState = DialogState_Done;
      } else {
        TIMInterpreterButtonClicked(&gameCtx->timInterpreter.timInterpreter, 0);
      }

    } else if (gameCtx->mouseEv.pos.x >= DIALOG_BUTTON2_X &&
               gameCtx->mouseEv.pos.x < DIALOG_BUTTON2_X + DIALOG_BUTTON_W) {
      printf("button 1\n");
      if (gameCtx->dialogState == DialogState_InProgress) {
        assert(0);
      } else {
        TIMInterpreterButtonClicked(&gameCtx->timInterpreter.timInterpreter, 1);
      }
    } else if (gameCtx->mouseEv.pos.x >= DIALOG_BUTTON3_X &&
               gameCtx->mouseEv.pos.x < DIALOG_BUTTON3_X + DIALOG_BUTTON_W) {
      printf("button 2\n");
      if (gameCtx->dialogState == DialogState_InProgress) {
        assert(0);
      } else {
        TIMInterpreterButtonClicked(&gameCtx->timInterpreter.timInterpreter, 2);
      }
    }
  }
  // printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
  return 0;
}

static int processMapViewMouse(GameContext *gameCtx) {
  if (zoneClicked(&gameCtx->mouseEv.pos, MAP_SCREEN_EXIT_BUTTON_X,
                  MAP_SCREEN_BUTTONS_Y, MAP_SCREEN_EXIT_BUTTON_W,
                  MAP_SCREEN_EXIT_BUTTON_H)) {
    GameContextSetState(gameCtx, GameState_PlayGame);
  } else if (zoneClicked(&gameCtx->mouseEv.pos, MAP_SCREEN_PREV_BUTTON_X,
                         MAP_SCREEN_BUTTONS_Y, MAP_SCREEN_PREV_NEXT_BUTTON_W,
                         MAP_SCREEN_PREV_NEXT_BUTTON_H)) {
    printf("PREV\n");
  } else if (zoneClicked(&gameCtx->mouseEv.pos, MAP_SCREEN_NEXT_BUTTON_X,
                         MAP_SCREEN_BUTTONS_Y, MAP_SCREEN_PREV_NEXT_BUTTON_W,
                         MAP_SCREEN_PREV_NEXT_BUTTON_H)) {
    printf("NEXT\n");
  } else {
    printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
  }
  return 1;
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
    int charIndex = charZoneClicked(gameCtx);
    if (charIndex != -1) {
      GameContextSetState(gameCtx, GameState_ShowInventory);
      gameCtx->selectedChar = charIndex;
      return 1;
    } else {
      return processCharInventoryItemsMouse(gameCtx);
    }
  }
  return 0;
}

static int tryMove(GameContext *gameCtx, Direction dir) {
  Orientation orientation = 0;
  switch (dir) {
  case Front:
    orientation = absOrientation(gameCtx->orientation, North);
    break;
  case Left:
    orientation = absOrientation(gameCtx->orientation, West);
    break;
  case Back:
    orientation = absOrientation(gameCtx->orientation, South);
    break;
  case Right:
    orientation = absOrientation(gameCtx->orientation, East);
    break;
  }
  uint16_t newBlock = BlockCalcNewPosition(gameCtx->currentBock, orientation);
  if (gameCtx->_noClip) {
    gameCtx->currentBock = newBlock;
    return 1;
  }

  Orientation facingOrientation = (orientation + 2) % 4;
  const MazeBlock *block =
      gameCtx->level->mazHandle.maze->wallMappingIndices + newBlock;
  uint8_t wmi = block->face[facingOrientation];
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  if (mapping == NULL || mapping->wallType == 0 || mapping->wallType == 3) {
    gameCtx->currentBock = newBlock;
    runBlockScript(gameCtx);
  } else {
    GameContextGetString(gameCtx, STR_CANT_GO_THAT_WAY_INDEX,
                         gameCtx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
  }
  return 1;
}

static int processCharZoneMouse(GameContext *gameCtx, int charIndex,
                                int coordX) {
  int prevSelectedChar = gameCtx->selectedChar;
  gameCtx->selectedChar = charIndex;
  if (gameCtx->selectedCharIsCastingSpell &&
      prevSelectedChar == gameCtx->selectedChar) {
    int spellLevel = (gameCtx->mouseEv.pos.y - CHAR_ZONE_Y) / 8;
    printf("actually perform magic %i: char %i\n", spellLevel,
           gameCtx->selectedChar);
    gameCtx->selectedCharIsCastingSpell = 0;
  } else if (zoneClicked(&gameCtx->mouseEv.pos, coordX, CHAR_ZONE_Y,
                         CHAR_FACE_W, CHAR_FACE_H)) {
    GameContextSetState(gameCtx, GameState_ShowInventory);
  } else if (zoneClicked(&gameCtx->mouseEv.pos, coordX + 44, CHAR_ZONE_Y, 22,
                         18)) {
    printf("Attack: char %i\n", gameCtx->selectedChar);
    gameCtx->selectedCharIsCastingSpell = 0;
  } else if (zoneClicked(&gameCtx->mouseEv.pos, coordX + 44, CHAR_ZONE_Y + 16,
                         22, 18)) {
    if (gameCtx->selectedCharIsCastingSpell &&
        prevSelectedChar == gameCtx->selectedChar) {

    } else if (gameCtx->selectedCharIsCastingSpell == 0) {
      gameCtx->selectedCharIsCastingSpell = 1;
    }
  } else {
    const SAVCharacter *c = gameCtx->chars + charIndex;
    GameRenderSetDialogF(gameCtx, 0X4047, c->name, c->hitPointsCur,
                         c->hitPointsMax, c->magicPointsCur, c->magicPointsMax);
  }
  return 1;
}

static int charZoneXcoord[][3] = {
    {CHAR_ZONE_0_1_X},
    {CHAR_ZONE_0_2_X, CHAR_ZONE_1_2_X},
    {CHAR_ZONE_0_3_X, CHAR_ZONE_1_3_X, CHAR_ZONE_2_3_X},
};

static int processCharZonesMouse(GameContext *gameCtx) {
  int charIndex = charZoneClicked(gameCtx);
  if (charIndex != -1) {
    uint8_t numChars = GameContextGetNumChars(gameCtx);
    int coordX = charZoneXcoord[numChars - 1][charIndex];
    return processCharZoneMouse(gameCtx, charIndex, coordX);
  } else {
    printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
  }
  return 0;
}

static int processPlayGameMouse(GameContext *gameCtx) {
  if (zoneClicked(&gameCtx->mouseEv.pos, UI_TURN_LEFT_BUTTON_X,
                  UI_TURN_LEFT_BUTTON_Y, UI_DIR_BUTTON_W * 3,
                  UI_DIR_BUTTON_W * 2)) {
    int x = gameCtx->mouseEv.pos.x - UI_TURN_LEFT_BUTTON_X;
    int y = gameCtx->mouseEv.pos.y - UI_TURN_LEFT_BUTTON_Y;
    int buttonX = (int)(x / UI_DIR_BUTTON_W);
    int buttonY = (int)(y / UI_DIR_BUTTON_W);
    if (buttonX <= 2 && buttonY <= 1) {
      if (buttonY == 0) {
        if (buttonX == 0) {
          gameCtx->orientation = OrientationTurnLeft(gameCtx->orientation);
        } else if (buttonX == 1) {
          tryMove(gameCtx, Front);
        } else if (buttonX == 2) {
          gameCtx->orientation = OrientationTurnRight(gameCtx->orientation);
        }
      } else if (buttonY == 1) {
        if (buttonX == 0) {
          tryMove(gameCtx, Left);
        } else if (buttonX == 1) {
          tryMove(gameCtx, Back);
        } else if (buttonX == 2) {
          tryMove(gameCtx, Right);
        }
      }
      return 1;
    }
  } else if (zoneClicked(&gameCtx->mouseEv.pos, UI_MENU_BUTTON_X,
                         UI_MENU_BUTTON_Y, UI_MENU_INV_BUTTON_W * 2,
                         UI_MENU_INV_BUTTON_H)) {
    int x = gameCtx->mouseEv.pos.x - UI_MENU_BUTTON_X;
    int buttonX = (int)(x / UI_MENU_INV_BUTTON_W);
    if (buttonX == 0) {
      GameContextSetState(gameCtx, GameState_GameMenu);
      return 1;
    } else if (buttonX == 1) {
      printf("sleep button\n");
    }
  } else if (mouseIsInInventoryStrip(gameCtx)) {
    return processInventoryStripMouse(gameCtx);
  } else if (zoneClicked(&gameCtx->mouseEv.pos, MAZE_COORDS_X, MAZE_COORDS_Y,
                         MAZE_COORDS_W, MAZE_COORDS_H)) {
    int x = gameCtx->mouseEv.pos.x - MAZE_COORDS_X;
    int y = gameCtx->mouseEv.pos.y - MAZE_COORDS_Y;
    printf("maze click %i %i\n", x, y);
    clickOnFrontWall(gameCtx);
    return 1;
  } else if (zoneClicked(&gameCtx->mouseEv.pos, UI_MAP_BUTTON_X,
                         UI_MAP_BUTTON_Y, UI_MAP_BUTTON_W, UI_MAP_BUTTON_H)) {
    GameContextSetState(gameCtx, GameState_ShowMap);
    AudioSystemPlaySoundFX(&gameCtx->audio, &gameCtx->sfxPak, "TURNPAG2.VOC");
    return 1;
  } else {
    return processCharZonesMouse(gameCtx);
  }
  return 0;
}

static int processMouse(GameContext *gameCtx) {
  switch (gameCtx->state) {
  case GameState_PlayGame:
    if (gameCtx->dialogState == DialogState_InProgress) {
      processAnimationMouse(gameCtx);
    }
    return processPlayGameMouse(gameCtx);
  case GameState_ShowMap:
    return processMapViewMouse(gameCtx);
  case GameState_ShowInventory:
    return processCharInventoryMouse(gameCtx);
  case GameState_TimAnimation:
    return processAnimationMouse(gameCtx);
  case GameState_GrowDialogBox:
    break;
  case GameState_MainMenu:
  case GameState_GameMenu: {
    int ret = MenuMouse(gameCtx->currentMenu, gameCtx, &gameCtx->mouseEv.pos);
    if (gameCtx->currentMenu->returnToGame) {
      GameContextSetState(gameCtx, GameState_PlayGame);
    }
    return ret;
  }

  case GameState_ShrinkDialogBox:
  case GameState_Invalid:
    assert(0);
    break;
  }
  return 0;
}

static void processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    assert(gameCtx->mouseEv.pending == 0); // prev one needs to be handled
    gameCtx->mouseEv.pending = 1;
    gameCtx->mouseEv.pos.x = e->motion.x / SCREEN_FACTOR;
    gameCtx->mouseEv.pos.y = e->motion.y / SCREEN_FACTOR;
    gameCtx->mouseEv.isRightClick = e->button.button == 3;
  } else if (e->type != SDL_KEYDOWN) {
    return;
  }

  if (gameCtx->state == GameState_GameMenu ||
      gameCtx->state == GameState_MainMenu) {
    int ret = MenuKeyDown(gameCtx->currentMenu, gameCtx, e);
    if (gameCtx->currentMenu->returnToGame) {
      GameContextSetState(gameCtx, GameState_PlayGame);
    }
    gameCtx->shouldUpdate = ret;
  } else if (gameCtx->state == GameState_ShowInventory ||
             gameCtx->state == GameState_ShowMap) {
    switch (e->key.keysym.sym) {
    case SDLK_ESCAPE:
      GameContextSetState(gameCtx, GameState_PlayGame);
      gameCtx->shouldUpdate = 1;
      break;
    }
  } else if (!gameCtx->controlDisabled && e->type == SDL_KEYDOWN) {
    switch (e->key.keysym.sym) {
    case SDLK_z:
      // go front
      gameCtx->shouldUpdate = 1;
      tryMove(gameCtx, Front);
      break;
    case SDLK_s:
      // go back
      gameCtx->shouldUpdate = 1;
      tryMove(gameCtx, Back);
      break;
    case SDLK_q:
      // go left
      gameCtx->shouldUpdate = 1;
      tryMove(gameCtx, Left);
      break;
    case SDLK_d:
      // go right
      gameCtx->shouldUpdate = 1;
      tryMove(gameCtx, Right);
      break;
    case SDLK_a:
      // turn anti-clockwise
      gameCtx->shouldUpdate = 1;
      gameCtx->orientation = OrientationTurnLeft(gameCtx->orientation);
      break;
    case SDLK_e:
      // turn clockwise
      gameCtx->shouldUpdate = 1;
      gameCtx->orientation = OrientationTurnRight(gameCtx->orientation);
      break;
    case SDLK_p:
      inspectFrontWall(gameCtx);
      break;
    case SDLK_SPACE:
      gameCtx->shouldUpdate = 1;
      runBlockScript(gameCtx);
      break;
    case SDLK_ESCAPE:
      GameContextSetState(gameCtx, GameState_GameMenu);
      gameCtx->shouldUpdate = 1;
      return;
    default:
      break;
    }
  }
}

static void GamePreUpdate(GameContext *gameCtx) {
  while (gameCtx->state == GameState_PlayGame &&
         EMCInterpreterIsValid(&gameCtx->interp, &gameCtx->interpState)) {
    EMCInterpreterRun(&gameCtx->interp, &gameCtx->interpState);
    gameCtx->shouldUpdate = 1;
    if (gameCtx->dialogState == DialogState_InProgress) {
      return;
    }
  }
  if (gameCtx->nextFunc) {
    printf("Exec next func %X\n", gameCtx->nextFunc);
    GameContextRunScript(gameCtx, gameCtx->nextFunc);
    gameCtx->nextFunc = 0;
  }
}

static void GameRunOnce(GameContext *gameCtx) {

  if (DBGServerUpdate(gameCtx)) {
    gameCtx->shouldUpdate = 1;
  }
  GamePreUpdate(gameCtx);

  SDL_Event e;
  SDL_WaitEventTimeout(&e, 20);
  if (e.type == SDL_QUIT) {
    gameCtx->_shouldRun = 0;
  }

  switch (gameCtx->state) {
  case GameState_PlayGame:
  case GameState_ShowInventory:
  case GameState_ShowMap:
  case GameState_MainMenu:
  case GameState_GameMenu:
    processGameInputs(gameCtx, &e);
    break;
  case GameState_TimAnimation:
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
      gameCtx->shouldUpdate = 1;
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
    gameCtx->shouldUpdate = 1;
    break;
  case GameState_Invalid:
    assert(0);
    break;
  }

  if (gameCtx->mouseEv.pending) {
    if (processMouse(gameCtx)) {
      gameCtx->shouldUpdate = 1;
    }
    gameCtx->mouseEv.pending = 0;
  }
  if (gameCtx->shouldUpdate) {
    GameRender(gameCtx);
    gameCtx->shouldUpdate = 0;
  }

  SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                   PIX_BUF_HEIGHT * SCREEN_FACTOR};
  assert(SDL_RenderCopy(gameCtx->renderer, gameCtx->pixBuf, NULL, &dest) == 0);
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
