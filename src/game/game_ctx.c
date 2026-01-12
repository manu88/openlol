#include "game_ctx.h"
#include "audio.h"
#include "bytes.h"
#include "config.h"
#include "dbg_server.h"
#include "display.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game.h"
#include "game_envir.h"
#include "game_render.h"
#include "game_strings.h"
#include "game_tim_animator.h"
#include "menu.h"
#include "pak_file.h"
#include "render.h"
#include "script.h"
#include "spells.h"
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

uint16_t GetRandom(uint16_t min, uint16_t max) {
  uint16_t r = (min + (uint16_t)arc4random()) % max;
  printf("GetRandom %i<=%i<%i\n", min, r, max);
  return r;
}

int GameContextSetSavDir(GameContext *gameCtx, const char *path) {
  if (path == NULL) {
    gameCtx->savDir = "./";
  } else {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
      assert(0);
      return 0;
    }
    if (S_ISDIR(path_stat.st_mode)) {
      gameCtx->savDir = (char *)path;
    } else {
      gameCtx->savDir = dirname((char *)path);
    }
  }
  return 1;
}

void SAVFilesRelease(SAVFile *files, size_t numFiles) {
  for (int i = 0; i < numFiles; i++) {
    free(files[i].fullpath);
    free(files[i].savName);
  }
  free(files);
}

SAVFile *GameContextListSavFiles(GameContext *gameCtx, size_t *numSavFiles) {
  DIR *dp;
  struct dirent *ep;
  dp = opendir(gameCtx->savDir);
  if (dp == NULL) {
    perror("Couldn't open the directory");
    assert(0);
  }

  const size_t fullPathSize = strlen(gameCtx->savDir) + 32;
  char *fullPath = malloc(fullPathSize);

  SAVFile *files = NULL;
  size_t num = 0;
  while ((ep = readdir(dp)) != NULL) {
    const char *dot = strrchr(ep->d_name, '.');
    if (!dot) {
      continue;
    }
    const char *ext = dot + 1;
    if (strcmp(ext, "DAT") != 0) {
      continue;
    }
    snprintf(fullPath, fullPathSize, "%s/%s", gameCtx->savDir, ep->d_name);
    size_t fileSize;
    size_t bufferSize;
    uint8_t *buffer = readBinaryFile(fullPath, &fileSize, &bufferSize);
    SAVHandle sav = {0};
    SAVHandleFromBuffer(&sav, buffer, bufferSize);

    num++;
    files = realloc(files, num * sizeof(SAVFile));
    assert(files);
    files[num - 1].fullpath = strdup(fullPath);
    files[num - 1].savName = strdup(sav.slot.header->name);

    free(buffer);
  }
  *numSavFiles = num;
  free(fullPath);
  closedir(dp);
  return files;
}

static Display _renderCtx = {0};

int GameContextInit(GameContext *gameCtx, Language lang) {
  memset(gameCtx, 0, sizeof(GameContext));
  gameCtx->display = &_renderCtx;
  DisplayInit(gameCtx->display);
  gameCtx->spellProperties = SpellPropertiesGet();
  if (!GameConfigFromFile(&gameCtx->conf, "conf.txt")) {
    printf("Create default config\n");
    GameConfigCreateDefault(&gameCtx->conf);
  }

  gameCtx->language = lang;
  GameContextSetState(gameCtx, GameState_MainMenu);

  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFileWithExt(
        &f, "LANDS", LanguageGetExtension(gameCtx->language)));
    if (LangHandleFromBuffer(&gameCtx->lang, f.buffer, f.bufferSize) == 0) {
      printf("unable to get LANDS.%s\n",
             LanguageGetExtension(gameCtx->language));
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFile(&f, "ITEM.INF"));
    if (INFScriptFromBuffer(&gameCtx->itemScript, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get ITEMS.INF\n");
      assert(0);
    }
  }

  assert(GameEnvironmentLoadPak(&gameCtx->defaultTlkFile, "00.TLK"));
  assert(GameEnvironmentLoadPak(&gameCtx->sfxPak, "VOC.PAK"));
  AudioSystemInit(&gameCtx->audio, &gameCtx->conf);

  AnimatorInit(&gameCtx->animator, gameCtx->display->pixBuf);
  GameTimInterpreterInit(&gameCtx->timInterpreter, &gameCtx->animator);
  gameCtx->timInterpreter.timInterpreter.callbackCtx = gameCtx;

  gameCtx->itemsInGame = malloc(MAX_IN_GAME_ITEMS * sizeof(GameObject));
  assert(gameCtx->itemsInGame);
  memset(gameCtx->itemsInGame, 0, MAX_IN_GAME_ITEMS * sizeof(GameObject));

  DBGServerInit();
  return 1;
}

int GameContextNewGame(GameContext *gameCtx) {
  gameCtx->levelId = 1;
  gameCtx->credits = 41;
  gameCtx->chars[0].id = -9; // Ak'shel for the win
  gameCtx->chars[0].flags = 1;
  snprintf(gameCtx->chars[0].name, 11, "Ak'shel");
  gameCtx->inventory[0] = GameContextCreateItem(gameCtx, 216); // salve
  gameCtx->inventory[1] = GameContextCreateItem(gameCtx, 217); // aloe
  gameCtx->inventory[2] = GameContextCreateItem(gameCtx, 218); // Ginseng
  GameContextLoadChars(gameCtx);
  if (GameContextLoadLevel(gameCtx, gameCtx->levelId)) {
    GameContextSetState(gameCtx, GameState_PlayGame);
    return 1;
  }
  return 0;
}

void GameContextRelease(GameContext *gameCtx) {
  DBGServerRelease();
  AudioSystemRelease(&gameCtx->audio);
  DisplayRelease(gameCtx->display);

  for (int i = 0; i < 3; i++) {
    if (gameCtx->display->buttonText[i]) {
      free(gameCtx->display->buttonText[i]);
    }
  }

  PAKFileRelease(&gameCtx->sfxPak);
  PAKFileRelease(&gameCtx->defaultTlkFile);
}

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId) {
  if (itemId == 0) {
    return 0;
  }

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    if (ctx->inventory[i] == 0) {
      ctx->inventory[i] = GameContextCreateItem(ctx, itemId);
      return 1;
    }
  }
  return 0;
}

static int runScript(GameContext *gameCtx, INFScript *script) {
  EMCState state = {0};
  EMCStateInit(&state, script);
  EMCStateSetOffset(&state, 0);
  EMCStateStart(&state, 0);
  while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
    EMCInterpreterRun(&gameCtx->interp, &state);
  }
  return 1;
}

static int runCompleteScript(GameContext *ctx, const char *name) {
  GameFile f;
  assert(GameEnvironmentGetStartupFile(&f, name));
  INFScript script = {0};
  assert(INFScriptFromBuffer(&script, f.buffer, f.bufferSize));
  return runScript(ctx, &script);
}

int GameContextStartup(GameContext *ctx) {
  return runCompleteScript(ctx, "ONETIME.INF");
}

void GameContextLoadLevelShapes(GameContext *gameCtx, const char *shpFile,
                                const char *datFile) {
  char pakFile[12] = "";
  strncpy(pakFile, shpFile, 12);

  pakFile[strlen(pakFile) - 1] = 'K';
  pakFile[strlen(pakFile) - 2] = 'A';
  pakFile[strlen(pakFile) - 3] = 'P';
  {
    GameFile f = {0};
    int ok = GameEnvironmentGetFileFromPak(&f, shpFile, pakFile);
    if (!ok) {
      ok = GameEnvironmentGetFile(&f, shpFile);
    }
    if (ok) {
      assert(SHPHandleFromBuffer(&gameCtx->level->shpHandle, f.buffer,
                                 f.bufferSize));
    }
  }
  {
    GameFile f = {0};
    int ok = GameEnvironmentGetFileFromPak(&f, datFile, pakFile);
    if (!ok) {
      ok = GameEnvironmentGetFile(&f, datFile);
    }
    if (ok) {
      assert(DatHandleFromBuffer(&gameCtx->level->datHandle, f.buffer,
                                 f.bufferSize));
    }
  }
}

static int runInitScript(GameContext *gameCtx, INFScript *script) {
  EMCState state = {0};
  EMCStateInit(&state, script);
  for (int i = 0; i < INFScriptGetNumFunctions(script); i++) {
    EMCStateStart(&state, i);
    while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
      EMCInterpreterRun(&gameCtx->interp, &state);
    }
  }
  return 1;
}

int GameContextLoadLevel(GameContext *ctx, int levelNum) {
  for (int i = 0; i < MAX_MONSTERS; i++) {
    MonsterInit(&ctx->level->monsters[i]);
  }
  DisplayResetDialog(ctx->display);

  GameEnvironmentLoadLevel(levelNum);
  {
    GameFile f = {0};
    char wllFile[12];
    snprintf(wllFile, 12, "LEVEL%i.WLL", levelNum);
    assert(GameEnvironmentGetFile(&f, wllFile));
    assert(WllHandleFromBuffer(&ctx->level->wllHandle, f.buffer, f.bufferSize));
  }

  {
    GameFile f = {0};
    char infFile[12];
    snprintf(infFile, 12, "LEVEL%i.INF", levelNum);
    assert(GameEnvironmentGetFile(&f, infFile));
    assert(INFScriptFromBuffer(&ctx->script, f.buffer, f.bufferSize));
  }
  {
    INFScript iniScript = {0};
    GameFile f = {0};
    char iniFile[12];
    snprintf(iniFile, 12, "LEVEL%i.INI", levelNum);
    assert(GameEnvironmentGetFile(&f, iniFile));
    assert(INFScriptFromBuffer(&iniScript, f.buffer, f.bufferSize));
    printf("->Run INI SCRIPT\n");
    runInitScript(ctx, &iniScript);
    printf("<-DONE INI SCRIPT\n");
  }
  {
    GameFile f = {0};
    char iniFile[12];
    snprintf(iniFile, 12, "LEVEL%i.XXX", levelNum);
    assert(GameEnvironmentGetFile(&f, iniFile));
    assert(
        XXXHandleFromBuffer(&ctx->level->legendData, f.buffer, f.bufferSize));
  }

  if (levelNum != ctx->level->currentTlkFileIndex) {
    AudioSystemClearVoiceQueue(&ctx->audio);
    GameContextLoadTLKFile(ctx, levelNum);
  }

  printf("->Run LEVEL INIT SCRIPT\n");
  GameContextRunLevelInitScript(ctx);
  printf("<-Done LEVEL INIT SCRIPT\n");
  return 1;
}

int GameContextLoadChars(GameContext *gameCtx) {
  char faceFile[11] = "";
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    uint8_t charId =
        gameCtx->chars[i].id > 0 ? gameCtx->chars[i].id : -gameCtx->chars[i].id;
    if (charId == 0) {
      continue;
    }
    assert(charId < 100);
    snprintf(faceFile, 11, "FACE%02i.SHP", charId);
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, faceFile));
    SHPHandleFromCompressedBuffer(&gameCtx->display->charFaces[i], f.buffer,
                                  f.bufferSize);
  }
  return 1;
}

int GameContextRunLevelInitScript(GameContext *gameCtx) {
  return GameContextRunScript(gameCtx, -1);
}

int GameContextRunScript(GameContext *gameCtx, int function) {
  EMCStateInit(&gameCtx->interpState, &gameCtx->script);
  EMCStateSetOffset(&gameCtx->interpState, 0);
  if (function > 0) {
    if (!EMCStateStart(&gameCtx->interpState, function)) {
      printf("EMCInterpreterStart: invalid\n");
      return 0;
    }
  }
  return 1;
}

int GameContextRunItemScript(GameContext *gameCtx, uint16_t charId,
                             uint16_t itemId, uint16_t flags, uint16_t next,
                             uint16_t reg4) {
  uint8_t func = 3;
  if (itemId) {
    func =
        gameCtx->itemProperties[gameCtx->itemsInGame[itemId].itemPropertyIndex]
            .scriptFun;
  }
  printf("func=%X\n", func);
  if (func == 0xFF) {
    return 0;
  }

  EMCState state = {0};
  EMCStateInit(&state, &gameCtx->itemScript);
  if (!EMCStateStart(&state, func)) {
    printf("EMCStateStart error\n");
    return 0;
  }

  state.regs[0] = flags;
  state.regs[1] = charId;
  state.regs[2] = itemId;
  state.regs[3] = next;
  state.regs[4] = next;

  while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
    EMCInterpreterRun(&gameCtx->interp, &state);
  }
  return 1;
}

uint16_t GameContextGetGameFlag(const GameContext *gameCtx, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(gameCtx->gameFlags));
  return ((gameCtx->gameFlags[flag >> 3] >> (flag & 7)) & 1);
}

void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(gameCtx->gameFlags));
  gameCtx->gameFlags[flag >> 3] |= (1 << (flag & 7));
}

void GameContextResetGameFlag(GameContext *gameCtx, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(gameCtx->gameFlags));
  gameCtx->gameFlags[flag >> 3] &= ~(1 << (flag & 7));
}

void GameContextSetState(GameContext *gameCtx, GameState newState) {
  gameCtx->prevState = gameCtx->state;
  gameCtx->state = newState;
  if (gameCtx->state == GameState_GameMenu) {
    DisplayCreateCursorForItem(gameCtx->display, 0);
    gameCtx->display->currentMenu = gameMenu;
  } else if (gameCtx->state == GameState_MainMenu) {
    gameCtx->display->currentMenu = mainMenu;
  } else if (gameCtx->display->currentMenu) {
    MenuReset(gameCtx->display->currentMenu);
    gameCtx->display->currentMenu = NULL;
  }
  if (gameCtx->state == GameState_ShowInventory ||
      gameCtx->state == GameState_ShowMap ||
      gameCtx->state == GameState_GameMenu) {
    gameCtx->selectedCharIsCastingSpell = 0;
    gameCtx->display->controlDisabled = 1;
  } else {
    gameCtx->display->controlDisabled = 0;
  }
  if (gameCtx->prevState == GameState_GameMenu &&
      gameCtx->state == GameState_PlayGame) {
    GameContextUpdateCursor(gameCtx);
  }
}

uint8_t GameContextGetNumChars(const GameContext *ctx) {
  uint8_t c = 0;
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    if (ctx->chars[i].id != 0) {
      c++;
    }
  }
  return c;
}

char *GameContextGetString3(const GameContext *gameCtx, uint16_t stringId) {
  char *str = GameContextGetString2(gameCtx, stringId);
  char *format = strdup(str);

  int placeholderIndex = 0;
  do {
    placeholderIndex = stringHasCharName(format, placeholderIndex);
    if (placeholderIndex != -1) {
      char *newFormat = stringReplaceHeroNameAt(
          gameCtx, format, DIALOG_BUFFER_SIZE, placeholderIndex);
      free(format);
      format = newFormat;
    }
  } while (placeholderIndex != -1);

  free(str);
  return format;
}

char *GameContextGetString2(const GameContext *ctx, uint16_t stringId) {
  char *s = malloc(DIALOG_BUFFER_SIZE);
  assert(s);
  GameContextGetString(ctx, stringId, s, DIALOG_BUFFER_SIZE);
  return s;
}

uint16_t GameContextGetString(const GameContext *ctx, uint16_t stringId,
                              char *outBuffer, size_t outBufferSize) {
  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(stringId, &useLevelFile);
  if (useLevelFile) {
    return LangHandleGetString(&ctx->level->levelLang, realStringId, outBuffer,
                               outBufferSize);
  }
  return LangHandleGetString(&ctx->lang, realStringId, outBuffer,
                             outBufferSize);
}

uint16_t GameContextGetLevelName(const GameContext *gameCtx, char *outBuffer,
                                 size_t outBufferSize) {
  return GameContextGetString(gameCtx,
                              STR_FIST_LEVEL_NAME_INDEX + gameCtx->levelId - 1,
                              outBuffer, outBufferSize);
}

uint16_t GameContextGetItemSHPFrameIndex(GameContext *gameCtx,
                                         uint16_t itemId) {
  return gameCtx->itemProperties[itemId].shapeId;
}

void GameContextInitSceneDialog(GameContext *gameCtx) {
  DisplayExpandDialogBox(gameCtx->display, gameCtx->conf.tickLength);
  gameCtx->display->controlDisabled = 1;
}

void GameContextCleanupSceneDialog(GameContext *gameCtx) {
  gameCtx->display->controlDisabled = 0;
  for (int i = 0; i < 3; i++) {
    if (gameCtx->display->buttonText[i]) {
      free(gameCtx->display->buttonText[i]);
      gameCtx->display->buttonText[i] = NULL;
    }
  }
  DisplayResetDialog(gameCtx->display);
  if (gameCtx->display->showBigDialog) {
    GameRender(gameCtx);
    DisplayShrinkDialogBox(gameCtx->display, gameCtx->conf.tickLength);
  }
  // FIXME: temporary clear screen here to avoid for cps background to remain on
  // screen
  gameCtx->display->showBitmap = 0;
}

uint16_t GameContextCreateItem(GameContext *gameCtx, uint16_t itemType) {
  int slot = -1;
  for (int i = 1; i < MAX_IN_GAME_ITEMS; i++) {
    GameObject *item = gameCtx->itemsInGame + i;
    if (item->itemPropertyIndex == 0) {
      slot = i;
      break;
    }
  }
  assert(slot != -1 && slot != MAX_IN_GAME_ITEMS);
  GameObject *item = gameCtx->itemsInGame + slot;
  item->itemPropertyIndex = itemType;
  return slot;
}

void GameContextDeleteItem(GameContext *gameCtx, uint16_t itemIndex) {
  memset(&gameCtx->itemsInGame[itemIndex], 0, sizeof(GameObject));
}

void GameContextUpdateCursor(GameContext *gameCtx) {
  uint16_t itemId =
      gameCtx->itemsInGame[gameCtx->itemIndexInHand].itemPropertyIndex;
  uint16_t frameId =
      itemId ? GameContextGetItemSHPFrameIndex(gameCtx, itemId) : 0;
  DisplayCreateCursorForItem(gameCtx->display, frameId);

  if (itemId == 0) {
    return;
  }

  char *itemName =
      GameContextGetString2(gameCtx, gameCtx->itemProperties[itemId].stringId);
  GameContextSetDialogF(gameCtx, STR_TAKEN_INDEX, itemName);
  free(itemName);
}

void GameContextExitGame(GameContext *gameCtx) { gameCtx->_shouldRun = 0; }

int GameContextLoadSaveFile(GameContext *gameCtx, const char *filepath) {
  SAVHandle savHandle = {0};
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    return 0;
  }
  if (!SAVHandleFromBuffer(&savHandle, buffer, readSize)) {
    return 0;
  }
  gameCtx->state = GameState_PlayGame;
  gameCtx->levelId = savHandle.slot.currentLevel;
  memcpy(gameCtx->itemsInGame, savHandle.slot.gameObjects,
         sizeof(GameObject) * MAX_IN_GAME_ITEMS);

  for (int i = 0; i < MAX_IN_GAME_ITEMS; i++) {
    const GameObject *obj = gameCtx->itemsInGame + i;
    if (obj->itemPropertyIndex == 0) {
      continue;
    }
  }
  memcpy(gameCtx->inventory, savHandle.slot.inventory,
         INVENTORY_SIZE * sizeof(uint16_t));

  for (int i = 0; i < NUM_CHARACTERS; i++) {
    if (!savHandle.slot.characters[i].flags) {
      memset(&gameCtx->chars[i], 0, sizeof(SAVCharacter));
      continue;
    }
    memcpy(&gameCtx->chars[i], &savHandle.slot.characters[i],
           sizeof(SAVCharacter));
  }
  gameCtx->currentBock = savHandle.slot.currentBlock;
  gameCtx->orientation = savHandle.slot.currentDirection;
  gameCtx->credits = savHandle.slot.credits;
  gameCtx->itemIndexInHand = savHandle.slot.itemIndexInHand;
  gameCtx->selectedChar = savHandle.slot.selectedChar;
  memcpy(gameCtx->globalScriptVars, savHandle.slot.globalScriptVars,
         NUM_GLOBAL_SCRIPT_VARS * 2);
  memset(gameCtx->gameFlags, 0, NUM_GAME_FLAGS);
  SAVHandleGetGameFlags(&savHandle, gameCtx->gameFlags, NUM_GAME_FLAGS);
  GameContextLoadLevel(gameCtx, gameCtx->levelId);
  GameContextUpdateCursor(gameCtx);
  GameContextLoadChars(gameCtx);
  return 1;
}

void GameContextLoadTLKFile(GameContext *gameCtx, int levelIndex) {
  printf("Load TLK File index %i\n", levelIndex);
  if (gameCtx->level->currentTlkFileIndex != -1) {
    PAKFileRelease(&gameCtx->level->currentTlkFile);
  }
  PAKFileInit(&gameCtx->level->currentTlkFile);

  static char tlkFilePath[7] = "";
  snprintf(tlkFilePath, 7, "%02d.TLK", levelIndex);
  printf("load TLK file '%s'\n", tlkFilePath);
  GameEnvironmentLoadPak(&gameCtx->level->currentTlkFile, tlkFilePath);
  gameCtx->level->currentTlkFileIndex = levelIndex;
}

static int getTLKSequence(GameContext *gameCtx, PAKFile *pak, int16_t charId,
                          uint16_t soundId, const char *pattern2,
                          int fileSequence[]) {

  int fileSequenceIndex = 0;
  char pattern1[8] = "";
  char file3[8] = "";
  if (soundId & 0x4000) {
    snprintf(pattern1, 8, "%03X", soundId & 0x3FFF);
  } else if (soundId < 1000) {
    snprintf(pattern1, 8, "%03d", soundId);
  } else {
    snprintf(file3, 8, "@%04d%c.%s", soundId - 1000, (char)charId, pattern2);
    int file3index = PakFileGetEntryIndex(pak, file3);
    if (file3index != -1) {
      fileSequence[fileSequenceIndex++] = file3index;
    }
  }

  char file1[9] = "";
  char file2[9] = "";
  if (strlen(file3) == 0) {
    for (char i = 0; i < 100; i++) {
      char symbol = '0' + i;
      snprintf(file1, 9, "%s%c%c.%s", pattern1, (char)charId, symbol, pattern2);
      snprintf(file2, 9, "%s%c%c.%s", pattern1, '_', symbol, pattern2);

      int file1index = PakFileGetEntryIndex(pak, file1);
      int file2index = PakFileGetEntryIndex(pak, file2);
      if (file1index != -1) {
        printf("found file1='%s'\n", file1);
        fileSequence[fileSequenceIndex++] = file1index;
      } else if (file2index != -1) {
        printf("found file2='%s'\n", file2);
        fileSequence[fileSequenceIndex++] = file2index;
      } else {
        break;
      }
    }
  }
  return fileSequenceIndex;
}

void GameContextPlayDialogSpeech(GameContext *gameCtx, int16_t charId,
                                 uint16_t soundId) {
  printf("GameContextPlayDialogSpeech charId=%X strId=%X\n", charId, soundId);
  // FIXME not sure this is the original algorithm to choose a speaker.
  uint8_t numChars = GameContextGetNumChars(gameCtx);
  if (charId > numChars) {
    charId = 0;
  }

  charId = (int16_t)gameCtx->chars[charId].name[0];

  char pattern2[8] = "";
  snprintf(pattern2, 8, "%02d",
           soundId & 0x4000 ? 0 : gameCtx->level->currentTlkFileIndex);

  int fileSequence[MAX_VOC_SEQ_ENTRIES] = {0};

  int fileSequenceIndex =
      getTLKSequence(gameCtx, &gameCtx->level->currentTlkFile, charId, soundId,
                     pattern2, fileSequence);

  if (fileSequenceIndex > 0) {
    AudioSystemPlayVoiceSequence(&gameCtx->audio,
                                 &gameCtx->level->currentTlkFile, fileSequence,
                                 fileSequenceIndex);
    return;
  }
  // is the sequence in default TLK file?
  fileSequenceIndex = getTLKSequence(gameCtx, &gameCtx->defaultTlkFile, charId,
                                     soundId, pattern2, fileSequence);

  if (fileSequenceIndex > 0) {
    AudioSystemPlayVoiceSequence(&gameCtx->audio, &gameCtx->defaultTlkFile,
                                 fileSequence, fileSequenceIndex);
  }
}

static const uint16_t sfxIndex[500] = {
    0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff,
    0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000,
    0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0x0034, 0x006f, 0x007c,
    0x0000, 0x007d, 0x0000, 0x007e, 0x0000, 0x007f, 0x0000, 0x0036, 0x0073,
    0x0035, 0xff9c, 0xffff, 0x0000, 0x0080, 0x0073, 0x0081, 0x0073, 0x0082,
    0x0073, 0x0083, 0x0073, 0x0084, 0x0073, 0x0085, 0x0073, 0x0086, 0x0073,
    0x00d7, 0x0076, 0x0087, 0x0065, 0x0038, 0x005f, 0x0037, 0x005f, 0x0032,
    0x000a, 0x0033, 0x0064, 0x0039, 0x0065, 0x003a, 0x000a, 0x003b, 0x0014,
    0x004e, 0x0065, 0x007a, 0x0064, 0x007b, 0x0064, 0x0088, 0x0065, 0x003c,
    0x0065, 0x003d, 0x0064, 0x0089, 0x0000, 0x008e, 0x0014, 0x003e, 0x0000,
    0x003f, 0x0000, 0x0040, 0x0065, 0x0041, 0x0000, 0x00c9, 0x0072, 0x00ca,
    0x0072, 0x00cb, 0x0072, 0x00cc, 0x0074, 0x00cd, 0x0074, 0x00ce, 0x0074,
    0x00cf, 0x0074, 0x00d0, 0x0073, 0x00d1, 0x0073, 0x00d2, 0x0073, 0x00d3,
    0x0073, 0x00d4, 0x0072, 0x00d5, 0x0072, 0x00d6, 0x0072, 0x0042, 0x006f,
    0x0043, 0x006f, 0x0044, 0x006f, 0x0045, 0x006f, 0x0046, 0x006f, 0x0047,
    0x006f, 0x0048, 0x0077, 0x0049, 0xff89, 0x004f, 0xff89, 0x0050, 0xff89,
    0x00a0, 0xff8a, 0x00a1, 0xff89, 0x00a2, 0xff89, 0xffff, 0x0000, 0x00a3,
    0xff89, 0x00a4, 0xff89, 0x004a, 0x0013, 0x004b, 0x0013, 0x004c, 0x0032,
    0x004d, 0x0032, 0x005f, 0x000a, 0x0051, 0x0000, 0x0052, 0x0000, 0x008f,
    0x0000, 0x0053, 0x0000, 0x0054, 0x0000, 0x0055, 0x0000, 0x0056, 0x0000,
    0x0057, 0x0000, 0x0058, 0x0077, 0x0060, 0x0000, 0x005d, 0x006a, 0x008a,
    0x006f, 0x008b, 0x006f, 0x008c, 0x0000, 0x008d, 0x0000, 0x005a, 0x0077,
    0x005c, 0x0000, 0x005b, 0x0000, 0x005e, 0x006f, 0x0061, 0x0076, 0xffff,
    0x0000, 0x0062, 0x0076, 0x0063, 0x003c, 0x0064, 0x006d, 0x0065, 0x0000,
    0x0066, 0x0000, 0x0067, 0x0000, 0x0068, 0x0000, 0x0069, 0x0000, 0x006a,
    0x0000, 0x006b, 0x0000, 0x006c, 0x0000, 0x006d, 0x0000, 0x006e, 0x0000,
    0x006f, 0x0000, 0x0070, 0xff88, 0xffff, 0x0000, 0x0071, 0x0000, 0x0072,
    0x0000, 0x0073, 0x0074, 0x0074, 0x0000, 0x0075, 0x0000, 0x0076, 0x0000,
    0x0077, 0x0000, 0x0078, 0x0000, 0x0079, 0x0077, 0x0090, 0x0000, 0x0091,
    0x0000, 0x0092, 0x0077, 0x0093, 0x001e, 0x0094, 0xff89, 0x0095, 0x0077,
    0x0096, 0x0076, 0x0097, 0x0072, 0x0098, 0x0000, 0x0099, 0x0050, 0x009a,
    0x0078, 0x009b, 0x0064, 0x009c, 0x005a, 0x009d, 0x0064, 0x009e, 0x0065,
    0x009f, 0x0066, 0x00a5, 0xff89, 0x00a6, 0xff89, 0x00a7, 0x0077, 0x00a8,
    0x0076, 0xffff, 0x0000, 0x00a9, 0x0077, 0x00aa, 0x0077, 0x00ab, 0x0077,
    0x00ac, 0x0077, 0x00ad, 0x0077, 0x00ae, 0x0077, 0x00af, 0x0076, 0x00b0,
    0x0077, 0x00b1, 0x0064, 0x00b2, 0x006f, 0x00b3, 0x006e, 0x00b4, 0x006e,
    0x00b5, 0x0077, 0x00b6, 0x0077, 0x00b7, 0x0077, 0x00b8, 0x0076, 0x00b9,
    0x0077, 0x00ba, 0x0077, 0x00bb, 0x0077, 0x00bc, 0x0077, 0x00bd, 0x0077,
    0x00be, 0x0077, 0x00bf, 0x0077, 0x00c0, 0x0076, 0x00c1, 0x0077, 0x00c2,
    0x0077, 0x00c3, 0x0077, 0x00c4, 0x006e, 0x00c5, 0x006e, 0x00c6, 0x007d,
    0x00c7, 0x0078, 0x00c8, 0x0078, 0x00d8, 0x0077, 0x00d9, 0x0077, 0x00da,
    0x0077, 0x00db, 0x0077, 0x00dc, 0x0077, 0x00dd, 0x0078, 0x00de, 0x0078,
    0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff,
    0x0000, 0x00e3, 0x0077, 0x00e4, 0x0077, 0x00e0, 0x001e, 0x00df, 0x0077,
    0x00e1, 0x0005, 0x00e2, 0x000a, 0x0000, 0x0050, 0x0001, 0x0050, 0x0002,
    0x0050, 0x0003, 0x0050, 0x0004, 0x0050, 0x0005, 0x0050, 0x0006, 0x0050,
    0x0007, 0x0050, 0x0008, 0x0050, 0x0009, 0x0050, 0x000a, 0x0050, 0x000b,
    0x0050, 0x000c, 0x0050, 0x000d, 0x0050, 0x000e, 0x0050, 0x000f, 0x0050,
    0x0010, 0x0050, 0x0011, 0x005a, 0x0012, 0x005a, 0x0013, 0x005a, 0x0014,
    0x005a, 0x0015, 0x005a, 0x0016, 0x005a, 0x0017, 0x005a, 0x0018, 0x005a,
    0x0019, 0x005a, 0x001a, 0x005a, 0x001b, 0x005a, 0x001c, 0x005a, 0x001d,
    0x005a, 0x001e, 0x005a, 0x001f, 0x005a, 0x0020, 0x005a, 0x0021, 0x0073,
    0x0022, 0x0073, 0x0023, 0x0073, 0x0024, 0x0073, 0x0025, 0x0073, 0x0026,
    0x0073, 0x0027, 0x0073, 0x0028, 0x0073, 0x0029, 0x0073, 0x002a, 0x0073,
    0x002b, 0x0050, 0x002c, 0x005a, 0x002d, 0x005a, 0x002e, 0x005a, 0x002f,
    0x005a, 0x0030, 0x005a, 0x0031, 0x005a};

static const char *const sfxNames[230] = {
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "XXXXXXXX",  "XXXXXXXX",  "XXXXXXXX", "XXXXXXXX",
    "XXXXXXXX", "XXXXXXXX", "DOORMTOP",  "DOORMTCL",  "PMETLHI",  "SWING6",
    "MDAMAGE2", "LEVERDN",  "LEVERUP",   "MTLDRSLM",  "DOORWDOP", "DOORWDCL",
    "KEYOPEN",  "KEYLOCK",  "PORTCUL",   "PORTCULD",  "LOKPICK",  "OCEAN2",
    "NUNCHUK",  "SWING",    "SHWING1",   "SWING6",    "THROW",    "CROSSBOW",
    "HEAL1",    "FIRESPL1", "PRESSIN",   "PRESSOUT",  "PLATEON",  "PLATEOFF",
    "DORWDSLM", "LITENIN1", "ICESPEL2",  "SPELL4B",   "SHIELD1",  "3BOLTC",
    "2BOLTC",   "1BOLTC",   "DAWNSPL1",  "HEALKING",  "SPELL7",   "SWING1",
    "EXPLODE",  "CROWCAW",  "MORPH2",    "CHEST",     "MONEY",    "SPELBK2",
    "AUTOMAP",  "MINECRT3", "CREAK1",    "TURNPAG2",  "POLGULP1", "GOOEY2",
    "BUCKBELL", "KEEPXIT2", "VSCREAM4",  "EMPTY",     "GOOEY1",   "GOOEY2",
    "RIPPOD4",  "PODSKEL1", "INVISO",    "OPENBOX2",  "ACCEPT2",  "BOW2",
    "HACHUCKM", "FOUNDRY2", "FOUNDRY2",  "FOUNDRY4",  "FOUNDRY6", "CLEANGL1",
    "CLEANGL2", "GLOWY1",   "DORSTNOP",  "DORSTNCL",  "EMPTY",    "EMPTY",
    "EMPTY",    "EMPTY",    "ADAMAGE1",  "HDAMAGE1",  "TDAMAGE1", "BDAMAGE1",
    "LDAMAGE1", "TDAMAGE2", "CDAMAGE1",  "EMPTY",     "EMPTY",    "EMPTY",
    "GOOD1",    "GOOD2",    "EMPTY",     "EMPTY",     "EMPTY",    "LITENIN1",
    "COMPASS2", "KINGDOR1", "GLASBRK2",  "FLUTTER3",  "NUNCHUK",  "WALLFALL",
    "WALLHIT",  "MWHOA1",   "LADDER",    "WHITTL3",   "ROWBOAT1", "HORSEY2",
    "SNORT",    "PUMPDOR1", "PUMPSM2",   "PUMPSM3",   "SPARK1",   "BEZEL",
    "SWARM",    "CHEST1",   "WRIT1",     "CAUSFOG",   "VAELAN2",  "ROARSPL1",
    "RATTLER",  "WINK1",    "HANDFATE",  "QUAKE1",    "WIZLAMP1", "SAP2",
    "MSTDOOM1", "GARDIAN1", "VORTEX1",   "LION1",     "STEAM",    "SQUAWCK",
    "SLIDEMUG", "SPARKHIT", "SPARKHIT2", "SPARKHIT3", "ICEFORM",  "ICEXPLOD",
    "EXPLODE2", "EXPLODE3", "BOLTQUK2",  "BOLT2",     "BOLT3",    "SNKBITE",
    "HANDGLOW", "MSTDOOM2", "MSTDOOM3",  "GARDIAN2",  "PLUSPOWR", "MINSPOWR",
    "BLUDCURL", "LORAGASP", "POURH2O",   "AWHOA2",    "HWHOA1",   "CWHOA1",
    "AFALL2",   "EMPTY",    "CFALL2",    "MFALL2",    "EMPTY",    "EMPTY",
    "EMPTY",    "EMPTY",    "EMPTY",     "EMPTY",     "EMPTY",    "EMPTY",
    "WRIT2",    "WRIT3",    "WRIT4",     "WRIT5",     "WRIT6",    "RUCKUS1",
    "RUCKUS3",  "CHANT1",   "EMPTY",     "EMPTY",     "EMPTY",    "CHANT2",
    "CHANT3",   ""};

void GameContextPlaySoundFX(GameContext *gameCtx, uint16_t soundId) {
  uint16_t index = sfxIndex[soundId << 1];

  const char *name = sfxNames[index];
  char fullName[16] = "";
  snprintf(fullName, 16, "%s.VOC", name);
  AudioSystemPlaySoundFX(&gameCtx->audio, &gameCtx->sfxPak, fullName);
}

void GameContextButtonClicked(GameContext *gameCtx, ButtonType button) {
  switch (button) {
  case ButtonType_Up:
    tryMove(gameCtx, Front);
    break;
  case ButtonType_Down:
    tryMove(gameCtx, Back);
    break;
  case ButtonType_Left:
    tryMove(gameCtx, Left);
    break;
  case ButtonType_Right:
    tryMove(gameCtx, Right);
    break;
  case ButtonType_TurnLeft:
    gameCtx->orientation = OrientationTurnLeft(gameCtx->orientation);
    break;
  case ButtonType_TurnRight:
    gameCtx->orientation = OrientationTurnRight(gameCtx->orientation);
    break;
  case ButtonType_Automap:
    GameContextSetState(gameCtx, GameState_ShowMap);
    AudioSystemPlaySoundFX(&gameCtx->audio, &gameCtx->sfxPak, "TURNPAG2.VOC");
    break;
  }
}

int GameContextCheckMagic(GameContext *gameCtx, uint16_t charId,
                          uint16_t spellNum, uint16_t spellLevel) {
  const SAVCharacter *ch = gameCtx->chars + charId;
  const SpellProperties *spellprop = gameCtx->spellProperties + spellNum;
  if (spellprop->mpRequired[spellLevel] > ch->magicPointsCur) {
    GameContextSetDialogF(gameCtx, 0X4043, ch->name);
  }
  return 0;
}
