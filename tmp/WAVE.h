#pragma once

// Waves Audio library - Software based library, mainly on the CPU.

#include "QM.h"
#include "UTL.h"

#ifdef WAVE_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

enum {
  WAVE_AFPLAY,
  WAVE_AFREWIND,
  WAVE_AFLOOP,
};

typedef struct {
  UINT samplesn; // Does not depend on channels, it's a sample
  UCHAR* samples;
} WAVE_AUDIODATA, *WAVE_AUDIO;

typedef struct {
  QM_V offset;
  UINT si; // Sample index
  WAVE_AUDIO audio;
  // If we assume the ear hears in a 1.0f volume, setting the volume of a source to 1.0f means the speaker will play at the volume of the entire system. but by default ears are set to 0.5f, so it's fine :)
  float volume;
  int status;
} WAVE_SOURCEDATA, *WAVE_SOURCE;

typedef struct {
  QM_V offset;
  QM_V angles;
  // Set to 0.5f, which essentially plays sounds it half their strength. Each individual time you play some sound or source it is multiplied by this value in realtime.
  float volume;
} WAVE_EARSDATA, *WAVE_EARS;

EXTERN WAVE_EARS WAVE_usedears;

// General WAVE
// worstfps is the worst fps you are gonna prepare yourself for, usually if you make a good game, it should not spike, but you may wanna 
// samplerate of 0 will simply initialize to the default sample rate.
int WAVE_init(int worstfps, int samplerate);
void WAVE_begin();
// Like playsource but works with music and stuff, the offset is relative to just 0,0,0 not the used ears.
void WAVE_playmusic(WAVE_SOURCE source);
void WAVE_playsource(WAVE_SOURCE source);
void WAVE_end();
void WAVE_free();

// Please do not load audio before init, since WAVE uses the device's information to reformat the audio(on load) incase it doesn't match.
WAVE_AUDIO WAVE_loadaudio(const char* fp);
WAVE_AUDIO WAVE_freeaudio(WAVE_AUDIO audio);

WAVE_SOURCE WAVE_initsource(WAVE_SOURCE source, int audiosn);
void WAVE_setaudio(WAVE_SOURCE source, int index, WAVE_AUDIO audio);
void WAVE_setaudioflag(WAVE_SOURCE source, int index, int flags);
// Remove flag
void WAVE_remaudioflags(WAVE_SOURCE source, int index, int flags);
void WAVE_setvolume(WAVE_SOURCE source, int index, float volume);
void WAVE_freesource(WAVE_SOURCE source);
