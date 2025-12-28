#pragma once
#include "formats/format_config.h"

#define CONF_KEY_SOUND_VOL "soundVol"
#define CONF_KEY_MUSIC_VOL "musicVol"
#define CONF_KEY_VOICE_VOL "voiceVol"
#define CONF_KEY_FPS "fps"

int GameConfigCreateDefault(ConfigHandle*handle);
