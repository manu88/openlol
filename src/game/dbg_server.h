#pragma once

typedef struct _GameContext GameContext;
int DBGServerInit(void);
void DBGServerRelease(void);
int DBGServerUpdate(GameContext *gameCtx);
