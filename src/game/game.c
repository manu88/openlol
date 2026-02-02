#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "dbg_server.h"
#include "display.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_wll.h"
#include "game_callbacks.h"
#include "game_char_inventory.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "game_strings.h"
#include "geometry.h"
#include "level.h"
#include "logger.h"
#include "menu.h"
#include "monster.h"
#include "prologue.h"
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
  LevelInit(gameCtx.level);

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

  GameConfigWriteFile(&gameCtx.conf, "conf.txt");
  GameContextRelease(&gameCtx);

  printf("GameEnvironmentRelease\n");
  GameEnvironmentRelease();
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static void inspectFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);

  Orientation facing = absOrientation(gameCtx->orientation, South);
  uint8_t wmi = gameCtx->level->blockProperties[nextBlock].walls[facing];

  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  if (mapping) {
    printf("clickOnFrontWall wmi=%i WallType wallType=0X%X decorationId=%X "
           "specialType=%X "
           "flags=%X automapData=%X\n",
           wmi, mapping->wallType, mapping->decorationId, mapping->specialType,
           mapping->flags, mapping->automapData);
  }
}

static void runBlockScript(GameContext *gameCtx) {
  GameContextRunScript(gameCtx, gameCtx->currentBock);
}

static void clickDoorSwitch(GameContext *gameCtx, uint16_t block) {
  printf("clickDoorSwitch\n");
  GameContextPlaySoundFX(gameCtx, 78);
  GameContextRunScript(gameCtx, block);
}

static void clickOnFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);

  Orientation facing = absOrientation(gameCtx->orientation, South);
  uint8_t wmi = gameCtx->level->blockProperties[nextBlock].walls[facing];

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
    GameContextRunScript(gameCtx, nextBlock);
    return;
  case WallSpecialType_LeverOn:
    printf("WallSpecialType_LeverOn\n");
    break;
  case WallSpecialType_LeverOff:
    printf("WallSpecialType_LeverOff\n");
    break;
  case WallSpecialType_OnlyScript:
    printf("WallSpecialType_OnlyScript\n");
    GameContextRunScript(gameCtx, nextBlock);
    return;
  case WallSpecialType_DoorSwitch:
    clickDoorSwitch(gameCtx, nextBlock);
    return;
  case WallSpecialType_Niche:
    printf("WallSpecialType_Niche\n");
    break;
  default:
    assert(0);
  }
}

static int charZoneClicked(const GameContext *gameCtx) {
  if (gameCtx->display->mouseEv.pos.y < CHAR_ZONE_Y ||
      gameCtx->display->mouseEv.pos.y > CHAR_ZONE_Y + CHAR_ZONE_H) {
    return -1;
  }

  uint8_t numChars = GameContextGetNumChars(gameCtx);
  switch (numChars) {
  case 1:
    if (gameCtx->display->mouseEv.pos.x > CHAR_ZONE_0_1_X &&
        gameCtx->display->mouseEv.pos.x < CHAR_ZONE_0_1_X + CHAR_ZONE_W) {
      return 0;
    }
    break;
  case 2: {
    if (gameCtx->display->mouseEv.pos.x > CHAR_ZONE_0_2_X &&
        gameCtx->display->mouseEv.pos.x < CHAR_ZONE_0_2_X + CHAR_ZONE_W) {
      return 0;
    } else if (gameCtx->display->mouseEv.pos.x > CHAR_ZONE_1_2_X &&
               gameCtx->display->mouseEv.pos.x <
                   CHAR_ZONE_1_2_X + CHAR_ZONE_W) {
      return 1;
    }
  } break;
  case 3: {
    if (gameCtx->display->mouseEv.pos.x > CHAR_ZONE_0_3_X &&
        gameCtx->display->mouseEv.pos.x < CHAR_ZONE_0_3_X + CHAR_ZONE_W) {
      return 0;
    } else if (gameCtx->display->mouseEv.pos.x > CHAR_ZONE_1_3_X &&
               gameCtx->display->mouseEv.pos.x <
                   CHAR_ZONE_1_3_X + CHAR_ZONE_W) {
      return 1;
    } else if (gameCtx->display->mouseEv.pos.x > CHAR_ZONE_2_3_X &&
               gameCtx->display->mouseEv.pos.x <
                   CHAR_ZONE_2_3_X + CHAR_ZONE_W) {
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
  return gameCtx->display->mouseEv.pos.x >= UI_INVENTORY_BUTTON_X &&
         gameCtx->display->mouseEv.pos.y >= UI_INVENTORY_BUTTON_Y &&
         gameCtx->display->mouseEv.pos.x <
             (UI_INVENTORY_BUTTON_X + (UI_MENU_INV_BUTTON_W * 11)) &&
         gameCtx->display->mouseEv.pos.y <
             (UI_INVENTORY_BUTTON_Y + (UI_MENU_INV_BUTTON_W * 1));
}

static int processInventoryStripMouse(GameContext *gameCtx) {
  int x = gameCtx->display->mouseEv.pos.x - UI_INVENTORY_BUTTON_X;
  int buttonX = (int)(x / UI_MENU_INV_BUTTON_W);
  // 0 is left arrow, 10 is right arrow
  if (buttonX == 0) {
    int inventoryIndex = gameCtx->inventoryIndex -
                         (gameCtx->display->mouseEv.isRightClick ? 9 : 1);
    if (inventoryIndex < 0) {
      inventoryIndex = INVENTORY_SIZE - 1;
    }
    gameCtx->inventoryIndex = inventoryIndex;
    return 1;
  } else if (buttonX == 10) {
    int inventoryIndex = gameCtx->inventoryIndex +
                         (gameCtx->display->mouseEv.isRightClick ? 9 : 1);
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
  if (gameCtx->display->mouseEv.pos.y >= DIALOG_BUTTON_Y_2 &&
      gameCtx->display->mouseEv.pos.y < DIALOG_BUTTON_Y_2 + DIALOG_BUTTON_H) {
    if (gameCtx->display->mouseEv.pos.x >= DIALOG_BUTTON1_X &&
        gameCtx->display->mouseEv.pos.x < DIALOG_BUTTON1_X + DIALOG_BUTTON_W) {
      printf("button 0\n");
      if (gameCtx->dialogState == DialogState_InProgress) {
        gameCtx->dialogState = DialogState_Done;
      } else {
        TIMInterpreterButtonClicked(&gameCtx->timInterpreter.timInterpreter, 0);
      }

    } else if (gameCtx->display->mouseEv.pos.x >= DIALOG_BUTTON2_X &&
               gameCtx->display->mouseEv.pos.x <
                   DIALOG_BUTTON2_X + DIALOG_BUTTON_W) {
      printf("button 1\n");
      if (gameCtx->dialogState == DialogState_InProgress) {
        assert(0);
      } else {
        TIMInterpreterButtonClicked(&gameCtx->timInterpreter.timInterpreter, 1);
      }
    } else if (gameCtx->display->mouseEv.pos.x >= DIALOG_BUTTON3_X &&
               gameCtx->display->mouseEv.pos.x <
                   DIALOG_BUTTON3_X + DIALOG_BUTTON_W) {
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
  if (zoneClicked(&gameCtx->display->mouseEv.pos, MAP_SCREEN_EXIT_BUTTON_X,
                  MAP_SCREEN_BUTTONS_Y, MAP_SCREEN_EXIT_BUTTON_W,
                  MAP_SCREEN_EXIT_BUTTON_H)) {
    GameContextSetState(gameCtx, GameState_PlayGame);
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos,
                         MAP_SCREEN_PREV_BUTTON_X, MAP_SCREEN_BUTTONS_Y,
                         MAP_SCREEN_PREV_NEXT_BUTTON_W,
                         MAP_SCREEN_PREV_NEXT_BUTTON_H)) {
    printf("PREV\n");
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos,
                         MAP_SCREEN_NEXT_BUTTON_X, MAP_SCREEN_BUTTONS_Y,
                         MAP_SCREEN_PREV_NEXT_BUTTON_W,
                         MAP_SCREEN_PREV_NEXT_BUTTON_H)) {
    printf("NEXT\n");
  } else {
    printf("mouse %i %i\n", gameCtx->display->mouseEv.pos.x,
           gameCtx->display->mouseEv.pos.y);
  }
  return 1;
}

static int processCharInventoryMouse(GameContext *gameCtx) {
  if (gameCtx->display->mouseEv.pos.x >= INVENTORY_SCREEN_EXIT_BUTTON_X &&
      gameCtx->display->mouseEv.pos.y >= INVENTORY_SCREEN_EXIT_BUTTON_Y &&
      gameCtx->display->mouseEv.pos.x <
          INVENTORY_SCREEN_EXIT_BUTTON_X + INVENTORY_SCREEN_EXIT_BUTTON_W &&
      gameCtx->display->mouseEv.pos.y <
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

int tryMove(GameContext *gameCtx, Direction dir) {
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
  if (gameCtx->conf.noClip) {
    gameCtx->currentBock = newBlock;
    return 1;
  }

  Orientation facingOrientation = (orientation + 2) % 4;
  uint8_t wmi =
      gameCtx->level->blockProperties[newBlock].walls[facingOrientation];
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  if (mapping == NULL || mapping->wallType == 0 || mapping->wallType == 3) {
    gameCtx->currentBock = newBlock;
    runBlockScript(gameCtx);
  } else {
    GameContextGetString(gameCtx, STR_CANT_GO_THAT_WAY_INDEX,
                         gameCtx->display->dialogTextBuffer,
                         DIALOG_BUFFER_SIZE);
  }
  return 1;
}

static void charUseItem(GameContext *gameCtx) {
  if (!gameCtx->itemIndexInHand) {
    return;
  }
  printf("charUseItem: char %i item 0X%X\n", gameCtx->selectedChar,
         gameCtx->itemIndexInHand);

  int flags =
      gameCtx
          ->itemProperties[gameCtx->itemsInGame[gameCtx->itemIndexInHand]
                               .itemPropertyIndex]
          .flags;
  printf("charUseItem: char %i item 0X%X flag=0X%X\n", gameCtx->selectedChar,
         gameCtx->itemIndexInHand, flags);
  if (flags & 1) {
    if (!(gameCtx->chars[gameCtx->selectedChar].flags & 8) || (flags & 0X20)) {
      printf("Run script\n");
      GameContextRunItemScript(gameCtx, gameCtx->selectedChar,
                               gameCtx->itemIndexInHand, 0X400, 0, 0);
    }
  }
}

static void charAttack(GameContext *gameCtx) {
  printf("Attack: char %i\n", gameCtx->selectedChar);
  SAVCharacter *c = gameCtx->chars + gameCtx->selectedChar;
  printf("char flags=0X%X\n", c->flags);

  int destBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);

  int monsterId = MonsterGetNearest(gameCtx, destBlock);
  printf("nearest %i\n", monsterId);
  int s = 0;
  for (int i = 0; i < 4; i++) {
    if (c->items[i] == 0) {
      continue;
    }

    GameContextRunItemScript(gameCtx, gameCtx->selectedChar, c->items[i], 0x400,
                             monsterId, s);
  }
}

static int processCharZoneMouse(GameContext *gameCtx, int charIndex,
                                int coordX) {
  int prevSelectedChar = gameCtx->selectedChar;
  gameCtx->selectedChar = charIndex;
  if (gameCtx->selectedCharIsCastingSpell &&
      prevSelectedChar == gameCtx->selectedChar) {
    int spellLevel = (gameCtx->display->mouseEv.pos.y - CHAR_ZONE_Y) / 8;
    printf("actually perform magic %i: char %i\n", spellLevel,
           gameCtx->selectedChar);
    gameCtx->selectedCharIsCastingSpell = 0;
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos, coordX, CHAR_ZONE_Y,
                         CHAR_FACE_W, CHAR_FACE_H)) {
    if (gameCtx->display->mouseEv.isRightClick) {
      charUseItem(gameCtx);
    } else {
      GameContextSetState(gameCtx, GameState_ShowInventory);
    }

  } else if (zoneClicked(&gameCtx->display->mouseEv.pos, coordX + 44,
                         CHAR_ZONE_Y, 22, 18)) {
    charAttack(gameCtx);
    gameCtx->selectedCharIsCastingSpell = 0;
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos, coordX + 44,
                         CHAR_ZONE_Y + 16, 22, 18)) {
    if (gameCtx->selectedCharIsCastingSpell &&
        prevSelectedChar == gameCtx->selectedChar) {

    } else if (gameCtx->selectedCharIsCastingSpell == 0) {
      gameCtx->selectedCharIsCastingSpell = 1;
    }
  } else {
    const SAVCharacter *c = gameCtx->chars + charIndex;
    GameContextSetDialogF(gameCtx, 0X4047, c->name, c->hitPointsCur,
                          c->hitPointsMax, c->magicPointsCur,
                          c->magicPointsMax);
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
    printf("mouse %i %i\n", gameCtx->display->mouseEv.pos.x,
           gameCtx->display->mouseEv.pos.y);
  }
  return 0;
}

static int processPlayGameMouse(GameContext *gameCtx) {
  if (zoneClicked(&gameCtx->display->mouseEv.pos, UI_TURN_LEFT_BUTTON_X,
                  UI_TURN_LEFT_BUTTON_Y, UI_DIR_BUTTON_W * 3,
                  UI_DIR_BUTTON_W * 2)) {
    int x = gameCtx->display->mouseEv.pos.x - UI_TURN_LEFT_BUTTON_X;
    int y = gameCtx->display->mouseEv.pos.y - UI_TURN_LEFT_BUTTON_Y;
    int buttonX = (int)(x / UI_DIR_BUTTON_W);
    int buttonY = (int)(y / UI_DIR_BUTTON_W);
    if (buttonX <= 2 && buttonY <= 1) {
      if (buttonY == 0) {
        if (buttonX == 0) {
          GameContextButtonClicked(gameCtx, ButtonType_TurnLeft);
        } else if (buttonX == 1) {
          GameContextButtonClicked(gameCtx, ButtonType_Up);
        } else if (buttonX == 2) {
          GameContextButtonClicked(gameCtx, ButtonType_TurnRight);
        }
      } else if (buttonY == 1) {
        if (buttonX == 0) {
          GameContextButtonClicked(gameCtx, ButtonType_Left);
        } else if (buttonX == 1) {
          GameContextButtonClicked(gameCtx, ButtonType_Down);
        } else if (buttonX == 2) {
          GameContextButtonClicked(gameCtx, ButtonType_Right);
        }
      }
      return 1;
    }
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos, UI_MENU_BUTTON_X,
                         UI_MENU_BUTTON_Y, UI_MENU_INV_BUTTON_W * 2,
                         UI_MENU_INV_BUTTON_H)) {
    int x = gameCtx->display->mouseEv.pos.x - UI_MENU_BUTTON_X;
    int buttonX = (int)(x / UI_MENU_INV_BUTTON_W);
    if (buttonX == 0) {
      GameContextSetState(gameCtx, GameState_GameMenu);
      return 1;
    } else if (buttonX == 1) {
      printf("sleep button\n");
    }
  } else if (mouseIsInInventoryStrip(gameCtx)) {
    return processInventoryStripMouse(gameCtx);
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos, MAZE_COORDS_X,
                         MAZE_COORDS_Y, MAZE_COORDS_W, MAZE_COORDS_H)) {
    int x = gameCtx->display->mouseEv.pos.x - MAZE_COORDS_X;
    int y = gameCtx->display->mouseEv.pos.y - MAZE_COORDS_Y;
    printf("maze click %i %i\n", x, y);
    clickOnFrontWall(gameCtx);
    return 1;
  } else if (zoneClicked(&gameCtx->display->mouseEv.pos, UI_MAP_BUTTON_X,
                         UI_MAP_BUTTON_Y, UI_MAP_BUTTON_W, UI_MAP_BUTTON_H)) {
    GameContextButtonClicked(gameCtx, ButtonType_Automap);
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
  case GameState_MainMenu:
  case GameState_GameMenu: {
    int ret = MenuMouse(gameCtx->display->currentMenu, gameCtx,
                        &gameCtx->display->mouseEv.pos);
    if (gameCtx->display->currentMenu->returnToGame) {
      GameContextSetState(gameCtx, GameState_PlayGame);
    }
    return ret;
  }
  case GameState_Prologue:
    printf("Ignoring mouse for now\n");
  case GameState_Invalid:
    assert(0);
    break;
  }
  return 0;
}

static void processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    assert(gameCtx->display->mouseEv.pending ==
           0); // prev one needs to be handled
    gameCtx->display->mouseEv.pending = 1;
    gameCtx->display->mouseEv.pos.x = e->motion.x / SCREEN_FACTOR;
    gameCtx->display->mouseEv.pos.y = e->motion.y / SCREEN_FACTOR;
    gameCtx->display->mouseEv.isRightClick = e->button.button == 3;
  } else if (e->type != SDL_KEYDOWN) {
    return;
  }

  if (gameCtx->state == GameState_GameMenu ||
      gameCtx->state == GameState_MainMenu) {
    int ret = MenuKeyDown(gameCtx->display->currentMenu, gameCtx, e);
    if (gameCtx->display->currentMenu->returnToGame) {
      GameContextSetState(gameCtx, GameState_PlayGame);
    }
    gameCtx->display->shouldUpdate = ret;
  } else if (gameCtx->state == GameState_ShowInventory ||
             gameCtx->state == GameState_ShowMap) {
    switch (e->key.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_TAB:
      GameContextSetState(gameCtx, GameState_PlayGame);
      gameCtx->display->shouldUpdate = 1;
      return;
    }
    if (!gameCtx->conf.moveInAutomap) {
      return;
    }
    switch (e->key.keysym.sym) {
    case SDLK_z:
      // go front
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Up);
      break;
    case SDLK_s:
      // go back
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Down);
      break;
    case SDLK_q:
      // go left
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Left);
      break;
    case SDLK_d:
      // go right
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Right);
      break;
    case SDLK_a:
      // turn anti-clockwise
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_TurnLeft);
      break;
    case SDLK_e:
      // turn clockwise
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_TurnRight);
      break;
    }
  } else if (!gameCtx->display->controlDisabled && e->type == SDL_KEYDOWN) {
    switch (e->key.keysym.sym) {
    case SDLK_z:
      // go front
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Up);
      break;
    case SDLK_s:
      // go back
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Down);
      break;
    case SDLK_q:
      // go left
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Left);
      break;
    case SDLK_d:
      // go right
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Right);
      break;
    case SDLK_a:
      // turn anti-clockwise
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_TurnLeft);
      break;
    case SDLK_e:
      // turn clockwise
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_TurnRight);
      break;
    case SDLK_p:
      inspectFrontWall(gameCtx);
      break;
    case SDLK_SPACE:
      gameCtx->display->shouldUpdate = 1;
      runBlockScript(gameCtx);
      break;
    case SDLK_TAB:
      gameCtx->display->shouldUpdate = 1;
      GameContextButtonClicked(gameCtx, ButtonType_Automap);
      break;
    case SDLK_ESCAPE:
      GameContextSetState(gameCtx, GameState_GameMenu);
      gameCtx->display->shouldUpdate = 1;
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
    gameCtx->display->shouldUpdate = 1;
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

static void getInputs(GameContext *gameCtx) {
  SDL_Event e;
  SDL_WaitEventTimeout(&e, gameCtx->conf.tickLength);
  if (e.type == SDL_QUIT) {
    gameCtx->_shouldRun = 0;
    return;
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
      gameCtx->display->shouldUpdate = 1;
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
      assert(gameCtx->display->mouseEv.pending ==
             0); // prev one needs to be handled
      gameCtx->display->mouseEv.pending = 1;
      gameCtx->display->mouseEv.pos.x = e.motion.x / SCREEN_FACTOR;
      gameCtx->display->mouseEv.pos.y = e.motion.y / SCREEN_FACTOR;
      gameCtx->display->mouseEv.isRightClick = e.button.button == 3;
    }
    break;
  case GameState_Prologue:
    printf("Ignoring game inputs for now\n");
    break;
  case GameState_Invalid:
    assert(0);
    break;
  }

  if (gameCtx->display->mouseEv.pending) {
    if (processMouse(gameCtx)) {
      gameCtx->display->shouldUpdate = 1;
    }
    gameCtx->display->mouseEv.pending = 0;
  }
}

static void GameRunOnce(GameContext *gameCtx) {
  if (DBGServerUpdate(gameCtx)) {
    gameCtx->display->shouldUpdate = 1;
  }

  if (gameCtx->state == GameState_Prologue) {
    int selectedChar = PrologueShow(gameCtx);
    if (gameCtx->_shouldRun == 0) {
      return;
    }
    printf("Selected char=%i\n", selectedChar);
    GameContextNewGame(gameCtx, selectedChar);
    gameCtx->display->shouldUpdate = 1;
  }

  getInputs(gameCtx);

  if (gameCtx->display->shouldUpdate) {
    GameRender(gameCtx);
    gameCtx->display->shouldUpdate = 0;
  }

  DisplayRender(gameCtx->display);
}

static int GameRun(GameContext *gameCtx) {
  gameCtx->_shouldRun = 1;
  while (gameCtx->_shouldRun) {
    GameRunOnce(gameCtx);
  }
  SDL_Quit();
  return 0;
}
