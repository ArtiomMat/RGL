// x86_64-w64-mingw32-gcc -ggdb -o waves WAVE.c -lmmdevapi -lole32

// pipewire-pulseaudio-0.3.76-1.fc37.x86_64 was removed, maybe will ruin stuff?

#include "WAVE.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <uuids.h>

#define DEFAULT_RATE 11025
// WARNING: If this is changed we must change the data type used for the buffer in ave_snd_PlayFrame()
#define DEFAULT_BYTE_DEPTH 1

// Either headers are broken for Windows & Linux or just Linux.
#undef DEFINE_GUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e,0x3d, 0xc4,0x57,0x92,0x91,0x69,0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2);
DEFINE_GUID(IID_IAudioRenderClient, 0xf294acfc, 0x3146, 0x4483, 0xa7,0xbf, 0xad,0xdc,0xa7,0xc2,0x60,0xe2);

// Used to enumerate devices.
static IMMDeviceEnumerator* pEnumerator; 
// The audio device I guess?
static IMMDevice* pDevice;
// The audio client, the interface between the device and us.
static IAudioClient* pClient;
// The audio render client, which is playing the audio and sends(maybe doesn't, no idea, not Microsoft) shit to the device.
static IAudioRenderClient* pRenderClient;

// The size of the buffer used to store audio frames(Frame=audio data at a point in time)
static UINT32 framesbuffersize;
static UINT* uiframesbuffer;

static int bytespersample;

#define CHECKR(msgstr) if (FAILED(r)) {puts("WAVE: " msgstr); fflush(stdout); return 0;}

int WAVE_init(int worstfps, int samplerate) {
  HRESULT r;

  r = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  CHECKR("Could not initialize COM.");

  r = CoCreateInstance(
    &CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
    &IID_IMMDeviceEnumerator, (void**)&pEnumerator
  );
  CHECKR("Could not create an enumerator.");

  r = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
    pEnumerator, // This
    eRender, eConsole, &pDevice
  );
  CHECKR("Could not retrieve device.");

  r = pDevice->lpVtbl->Activate(
	  pDevice, // This
    &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pClient
  );
  CHECKR("Could not activate device client.");
  
  WAVEFORMATEX* pDefaultWaveFormat = NULL;
  r = pClient->lpVtbl->GetMixFormat(pClient, &pDefaultWaveFormat);

  WAVEFORMATEX waveFormat = {0};
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = 2;
  if (samplerate)
    waveFormat.nSamplesPerSec = samplerate;
  else
    waveFormat.nSamplesPerSec = pDefaultWaveFormat->nSamplesPerSec;
  waveFormat.wBitsPerSample = pDefaultWaveFormat->wBitsPerSample==8?8:16; // Stutters and shifts the audio of the entire os if bps don't match up(specifically when I did 8 instead of 16). The default wave format(atleast for me) has 32 bps, which apparantly does not work with WAVE_FORMAT_PCM(1)? the format that works is WAVE_FORMAT_EXTENSIBLE(65534), no idea what it even is, don't wanna know, just gonna keep it like this and hope for the best.
  waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
  waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
  waveFormat.cbSize = pDefaultWaveFormat->cbSize;

  bytespersample = waveFormat.wBitsPerSample / 8;
  
  int framebuffertime = 10000000/worstfps;
  r = pClient->lpVtbl->Initialize(
    pClient, // This
    AUDCLNT_SHAREMODE_SHARED, 
    0, // AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 
    framebuffertime, 0, &waveFormat, NULL
  );
  CHECKR("Could not initialize the client.");

  (void)pClient->lpVtbl->GetBufferSize(
    pClient, // This 
    &framesbuffersize
  );
  
  r = pClient->lpVtbl->GetService(
		pClient, // This
		&IID_IAudioRenderClient, (void**)&pRenderClient
	);
  CHECKR("Could not get the render client.");
  
  r = pClient->lpVtbl->Start(pClient);
	CHECKR("Could not start the audio stream.");

  uiframesbuffer = malloc(sizeof(UINT) * framesbuffersize);

  printf("WAVE: Waves Audio library initialized. Frames buffer size is %d.", framesbuffersize);

  return 0;
}

static int writeframesn;
static BYTE* framesbuffer;
static int audiosplayedn;

void WAVE_begin() {
  UINT padding;

  pClient->lpVtbl->GetCurrentPadding(pClient, &padding);
  writeframesn = framesbuffersize - padding;

  pRenderClient->lpVtbl->GetBuffer(pRenderClient, writeframesn, (BYTE**)&framesbuffer);
  
  // Clear the to-be played buffer.
  for (int i = 0; i < writeframesn; i++)
    uiframesbuffer[i] = 0;
}

void WAVE_end() {
  for (int i = 0; i < writeframesn; i++) {
    framesbuffer[i] = WAVE_usedears->volume * uiframesbuffer[i] / audiosplayedn;
  }
  pRenderClient->lpVtbl->ReleaseBuffer(pRenderClient, writeframesn, 0);
}

void WAVE_playmusic(WAVE_SOURCE source) {
  for (int si = 0; si < writeframesn * 2; si+=2, source->si+=2) {
      if (source->si )
      switch (bytespersample) {
        case 2:
        USHORT* audiosampleptr = source->audio->samples;
        uiframesbuffer[si+0] += audiosampleptr[source->si+0] * source->volume;
        uiframesbuffer[si+1] += audiosampleptr[source->si+1] * source->volume;
        break;
        case 1:
        UCHAR* framesbuffer2 = framesbuffer;
        UCHAR* framesbuffer2 = framesbuffer;
        break;
      }
  }
  audiosplayedn++;
}

void WAVE_free() {
  pRenderClient->lpVtbl->Release(pRenderClient);

  pClient->lpVtbl->Stop(pClient);
  pClient->lpVtbl->Release(pClient);

  pDevice->lpVtbl->Release(pDevice);

  pEnumerator->lpVtbl->Release(pEnumerator);

  CoUninitialize();
}

WAVE_AUDIO WAVE_loadaudio(const char* fp) {
  FILE* f = fopen(fp, "rb");
	// NOTE: We already read the "RIFF" part.
	// Skipping: Size in bytes + Wave marker + Fmt marker + Fmt header length
	fseek(f, 4+4+4+4, SEEK_CUR);
	
	USHORT format;
	fread(&format, 2, 1, f);
	UTL_ILilE16(&format);
	// Not PCM?
	if (format != 1) {
		printf("WAVE: Could not recognize format of '%f.");
    fclose(f);
		return 0;
	}
	
	USHORT channelsn;
	fread(&channelsn, 2, 1, f);
	UTL_ILilE16(&channelsn);

	UINT samplerate;
	fread(&samplerate, 4, 1, f);
	UTL_ILilE32(&samplerate);

	// Skipping: byte rate + block alignment
	fseek(f, 4+2, SEEK_CUR);

	USHORT loadedbps;
	fread(&loadedbps, 2, 1, f);
	UTL_ILilE16(&loadedbps);

	// Skipping: Data marker
	fseek(f, 4, SEEK_CUR);

	UINT datasize;
	fread(&datasize, 4, 1, f);
	UTL_ILilE32(&datasize);

	UINT samplesn = datasize / (channelsn * loadedbps/8);
	UCHAR* samples = malloc(samplesn);

	// TODO: Convert shit if needed to.
	fread(samples, 1, samplesn, f);

  fclose(f);

	return ;
}

int main() {
  if (!WAVE_init(6, 0))
    return 1;
  printf("OK\n");
  fflush(stdout);
  while (1) {
    WAVE_begin();
    WAVE_end();
    Sleep(16);
  }
  WAVE_free();
  return 0;
}

