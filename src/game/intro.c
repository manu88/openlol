#include "intro.h"
#include "audio.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "tim.h"
#include "tim_interpreter.h"
#include "ui.h"
#include <_string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEXT_BUFFER_SIZE 128
static char textBuffer[TEXT_BUFFER_SIZE];

typedef struct {
  uint16_t index;
  char *file;
} VoiceEntry;

#define NUM_VOICE_ENTRIES 10
typedef struct {
  TIMContext timCtx;
  PAKFile voicePak;
  LangHandle lang; // LOLINTRO.DIP
  FNTHandle font9p;

  TIMInterpreterCallbacks baseTimCallbacks;

  int gameEnvirScopeMark;

  VoiceEntry voiceEntries[NUM_VOICE_ENTRIES];
} IntroContext;

static void VoiceEntryAdd(IntroContext *introCtx, uint16_t index,
                          const char *file) {
  for (int i = 0; i < NUM_VOICE_ENTRIES; i++) {
    if (introCtx->voiceEntries[i].index == 0) {
      introCtx->voiceEntries[i].index = index;
      introCtx->voiceEntries[i].file = strdup(file);
      return;
    }
  }
  assert(0);
}

static const char *VoiceEntryGet(IntroContext *introCtx, uint16_t index) {
  for (int i = 0; i < NUM_VOICE_ENTRIES; i++) {
    if (introCtx->voiceEntries[i].index == index) {
      return introCtx->voiceEntries[i].file;
    }
  }
  return NULL;
}

static void loadVocFile(TIMInterpreter *interp, const char *file,
                        uint16_t index) {
  IntroContext *intro = (IntroContext *)interp;
  VoiceEntryAdd(intro, index, file);
}

static void playVoc(TIMInterpreter *interp, uint16_t index, uint16_t volume) {
  IntroContext *intro = (IntroContext *)interp;

  const char *file = VoiceEntryGet(intro, index);
  assert(file);
  int seqId = PakFileGetEntryIndex(&intro->voicePak, file);
  assert(seqId != -1);
  AudioSystemPlayVoiceSequence(&intro->timCtx.gameCtx->audio, &intro->voicePak,
                               &seqId, 1);
}

static void callbackSetupPaletteFade(TIMInterpreter *interp, uint16_t param) {
  printf("callbackSetupPaletteFade\n");
}

static void callbackLoadPalette(TIMInterpreter *interp, const char *file) {
  printf("callbackLoadPalette %s\n", file);
}

static void callbackSetupPaletteFadeEx(TIMInterpreter *interp, uint16_t param) {
  printf("callbackSetupPaletteFadeEx\n");
}

static void callbackProcessWSAFrame(TIMInterpreter *interp, uint16_t index,
                                    uint16_t frame, uint16_t x, uint16_t y,
                                    uint16_t factor) {
  printf("callbackProcessWSAFrame\n");
}
static void callbackDisplayText(TIMInterpreter *interp, uint16_t textId,
                                uint16_t flags) {
  IntroContext *intro = (IntroContext *)interp;
  LangHandleGetString(&intro->lang, textId, textBuffer, TEXT_BUFFER_SIZE);

  printf("callbackDisplayText index=0X%X flags=0X%X '%s'\n", textId, flags,
         textBuffer);
  UIRenderText(&intro->font9p, intro->timCtx.gameCtx->display->pixBuf, 8, 48,
               100, textBuffer);
}

static void IntroInit(GameContext *gameCtx, IntroContext *introCtx) {
  printf("IntroInit\n");
  memset(introCtx, 0, sizeof(IntroContext));
  introCtx->gameEnvirScopeMark = GameEnvironmentAddScopeMark();
  assert(introCtx->gameEnvirScopeMark);

  assert(GameEnvironmentPreloadLocalizedPak("INTRO1.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO2.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO3.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO4.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO5.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO6.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO7.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTRO8.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("STARTUP.PAK"));

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FONT9PN.FNT"));

    if (FNTHandleFromBuffer(&introCtx->font9p, f.buffer, f.bufferSize) == 0) {
      printf("unable to get FONT9PN.FNT data\n");
    }
  }

  PAKFileInit(&introCtx->voicePak);
  assert(GameEnvironmentLoadLocalizedPak(&introCtx->voicePak, "INTROVOC.PAK"));

  TIMInit(&introCtx->timCtx, gameCtx, TIMInterpreterMode_Intro);

  introCtx->timCtx.interp.callbacks.TIMInterpreterCallbacks_LoadVocFile =
      loadVocFile;
  introCtx->timCtx.interp.callbacks.TIMInterpreterCallbacks_PlayVocFile =
      playVoc;

  introCtx->timCtx.interp.introCallbacks = (TIMInterpreterIntroCallbacks){
      .TIMInterpreterIntroCallbacks_SetupPaletteFade = callbackSetupPaletteFade,
      .TIMInterpreterIntroCallbacks_LoadPalette = callbackLoadPalette,
      .TIMInterpreterIntroCallbacks_SetupPaletteFadeEx =
          callbackSetupPaletteFadeEx,
      .TIMInterpreterIntroCallbacks_ProcessWSAFrame = callbackProcessWSAFrame,
      .TIMInterpreterIntroCallbacks_DisplayText = callbackDisplayText,
  };
  // introCtx->timCtx.interp.callbacks.TIMInterpreterCallbacks_LoadPalette =
  // loadPalette;

  TIMLoad(&introCtx->timCtx, 0, "LOLINTRO");

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "LOLINTRO.DIP"));
    assert(LangHandleFromBuffer(&introCtx->lang, f.buffer, f.bufferSize));
  }
}

static void IntroRelease(GameContext *gameCtx, IntroContext *introCtx) {
  GameEnvironmentUnloadTopMark(introCtx->gameEnvirScopeMark);
  PAKFileRelease(&introCtx->voicePak);
  AudioSystemClearVoiceQueue(&gameCtx->audio);
}

static void IntroMainLoop(GameContext *gameCtx, IntroContext *introCtx);

void IntroductionShow(GameContext *gameCtx) {
  IntroContext introCtx = {0};
  IntroInit(gameCtx, &introCtx);
  IntroMainLoop(gameCtx, &introCtx);
  IntroRelease(gameCtx, &introCtx);
}

static void IntroMainLoop(GameContext *gameCtx, IntroContext *introCtx) {
  TIMRun(&introCtx->timCtx, 0, 0);
}
