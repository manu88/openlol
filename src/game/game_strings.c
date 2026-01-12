#include "game_strings.h"

int stringHasCharName(const char *s, int startIndex) {
  size_t strLen = strlen(s);
  int i = 0;
  if (startIndex) {
    i = startIndex + 1;
  }
  for (; i < strLen; i++) {
    if (s[i] == '%') {
      if (i + 1 >= strLen) {
        return -1;
      }
      char next = s[i + 1];
      if (next == 'n') {
        return i;
      } // FIXME: handle other special string aliasing types here if any
    }
  }
  return -1;
}

char *stringReplaceHeroNameAt(const GameContext *gameCtx, char *string,
                              size_t bufferSize, int index) {
  const char *heroName = gameCtx->chars[0].name;
  size_t totalNewSize = strlen(string) - 2 + strlen(heroName) + 1;
  assert(totalNewSize < bufferSize);
  char *newStr = malloc(totalNewSize);
  memcpy(newStr, string, index);
  newStr[index] = 0;

  strcat(newStr, heroName);
  strcat(newStr, string + index + 2);
  return newStr;
}

void GameContextSetDialogF(GameContext *gameCtx, int stringId, ...) {
  GameContextGetString(gameCtx, stringId, gameCtx->renderer->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  char *format = strdup(gameCtx->renderer->dialogTextBuffer);
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

  va_list args;
  va_start(args, stringId);
  vsnprintf(gameCtx->renderer->dialogTextBuffer, DIALOG_BUFFER_SIZE, format,
            args);
  va_end(args);
  free(format);
  gameCtx->renderer->dialogText = gameCtx->renderer->dialogTextBuffer;
}
