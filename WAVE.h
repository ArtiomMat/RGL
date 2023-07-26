#pragma once

// Waves Audio library - 

#include "QM.h"

#ifdef WAVE_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

// WAVE - Sound library

typedef struct {
  
} WAVE_AUDIODATA, *WAVE_AUDIO;

typedef struct {
  QM_V offset;
} WAVE_SOURCEDATA, *WAVE_SOURCE;

typedef struct {
  QM_V offset;
  QM_V angles;

} WAVE_EARDATA, *WAVE_EAR;

// Set to 0.5f, which essentially plays sounds it half the full strength. Each individual time you play some sound or source it is multiplied by this value in realtime.
EXTERN float volume;

int WAVE_init();
void WAVE_begin();
void WAVE_playaudio(WAVE_AUDIO audio);
void WAVE_playsource(WAVE_SOURCE source);
void WAVE_end();
void WAVE_free();

