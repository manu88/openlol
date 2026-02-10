#pragma once
#include "formats/format_xmi.h"

/*
This unit is a wrapper around ymfm code, because it's a C++ codebase.
*/
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MusicPlayer MusicPlayer;
MusicPlayer *MusicPlayerCreate(void);
void MusicPlayerRelease(MusicPlayer *player);

int MusicPlayerLoadSequence(MusicPlayer *player, const XMIHandle *handle,
                            int trackId);
void MusicPlayerGenerate(MusicPlayer *_player, int16_t *stream,
                         unsigned numSamples);

int MusicMainLoop(const XMIHandle *handle, int trackId);

#ifdef __cplusplus
}
#endif
