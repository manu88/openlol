#include "console.h"
#include "game.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static char cmdBuffer[1024];

void setupConsole(GameContext *gameCtx) { gameCtx->cmdBuffer = cmdBuffer; }

static void doExec(GameContext *gameCtx, int argc, char *argv[]) {
  assert(argc);
  if (strcmp(argv[0], "exec") == 0) {
    int function = atoi(argv[1]);
    printf("script exec function %i\n", function);
    runScript(gameCtx, function);
  }
}

static void execCmd(GameContext *gameCtx, const char *cmd) {
  char *str = (char *)cmd;
  char *tok = NULL;
  int argc = 0;
  char **argv = NULL;
  while ((tok = strsep(&str, " ")) != NULL) {
    argv = realloc(argv, (argc + 1) * sizeof(char *));
    argv[argc] = tok;
    argc++;
  }
  if (argc) {
    doExec(gameCtx, argc, argv);
  }
  free(argv);
}

int processConsoleInputs(GameContext *gameCtx, const SDL_Event *e) {
  assert(SDL_IsTextInputActive());
  if (e->type == SDL_KEYDOWN) {
    if (e->key.keysym.sym == SDLK_ESCAPE) {
      gameCtx->consoleHasFocus = 0;
      printf("reset console focus\n");
      SDL_StopTextInput();
      return 1;
    } else if (e->key.keysym.sym == SDLK_BACKSPACE) {
      if (strlen(cmdBuffer) > 2) {
        cmdBuffer[strlen(cmdBuffer) - 1] = 0;
      }
    } else if (e->key.keysym.sym == SDLK_RETURN) {

      execCmd(gameCtx, cmdBuffer + 2);
      snprintf(cmdBuffer, 1024, "> ");
    }
  }

  if (e->type == SDL_TEXTINPUT) {
    strcat(cmdBuffer, e->text.text);
  }
  return 1;
}
