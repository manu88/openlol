#pragma once


typedef struct _GameContext GameContext;
int DBGServerInit(void);
void DBGServerRelease(void);
void DBGServerUpdate(const GameContext* gameCtx);
