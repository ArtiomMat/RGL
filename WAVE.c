// x86_64-w64-mingw32-gcc -ggdb -o waves WAVE.c -lmmdevapi -lole32

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

static WAVEFORMATEX WaveFormat = {
  .wFormatTag = WAVE_FORMAT_PCM,
  .nChannels = 1,
  .nSamplesPerSec = DEFAULT_RATE,
  .nAvgBytesPerSec = DEFAULT_RATE*DEFAULT_BYTE_DEPTH, // WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign
  .nBlockAlign = DEFAULT_BYTE_DEPTH, // WaveFormat.nChannels * (WaveFormat.wBitsPerSample / 8)
  .wBitsPerSample = DEFAULT_BYTE_DEPTH*8,
  .cbSize = 0, // Must be 0 with this configuration.
};

// The size of the buffer used to store audio frames(Frame=audio data at a point in time)
static UINT32 FramesN;

#define CHECKR(msgstr) if (FAILED(r)) {puts("WAVE: " msgstr); return 0;}

int WAVE_init() {
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
  CHECKR("Could not activate device.");


  WAVEFORMATEX* pDefaultMixFormat = NULL;
  r = pClient->lpVtbl->GetMixFormat(pClient, &pDefaultMixFormat);

  WAVEFORMATEX mixFormat = {};
  mixFormat.wFormatTag = WAVE_FORMAT_PCM;
  mixFormat.nChannels = 2;
  mixFormat.nSamplesPerSec = DEFAULT_RATE;//defaultMixFormat->nSamplesPerSec;
  mixFormat.wBitsPerSample = 16;
  mixFormat.nBlockAlign = (mixFormat.nChannels * mixFormat.wBitsPerSample) / 8;
  mixFormat.nAvgBytesPerSec = mixFormat.nSamplesPerSec * mixFormat.nBlockAlign;

  int framebuffertime = 10000000; // In hundreds of nano seconds, 40ms=25fps.
  r = pClient->lpVtbl->Initialize(
    pClient, // This
    AUDCLNT_SHAREMODE_SHARED, 
    0, // AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 
    framebuffertime, 0, &mixFormat, NULL
  );
  CHECKR("Could not initialize the client.");

  (void)pClient->lpVtbl->GetBufferSize(
    pClient, // This 
    &FramesN
  );
  
  r = pClient->lpVtbl->GetService(
		pClient, // This
		&IID_IAudioRenderClient, (void**)&pRenderClient
	);
  CHECKR("Could not get the render client.");
  
  r = pClient->lpVtbl->Start(pClient);
	CHECKR("Could not start the audio stream.");

  printf("WAVE: Waves Audio library initialized. Frames buffer size is %d.", FramesN);

  return 0;
}

void WAVE_begin() {

}

void WAVE_playsound() {

}

void WAVE_playsource() {

}

void WAVE_end() {
  
}

void WAVE_free() {
  pRenderClient->lpVtbl->Release(pRenderClient);

  pClient->lpVtbl->Stop(pClient);
  pClient->lpVtbl->Release(pClient);

  pDevice->lpVtbl->Release(pDevice);

  pEnumerator->lpVtbl->Release(pEnumerator);

  CoUninitialize();
}

int main() {
  WAVE_init();
  BYTE* frames;
  
  pRenderClient->lpVtbl->GetBuffer(pRenderClient, FramesN, &frames);
  for (int i = 0; i < FramesN; i++)
	  frames[i] = 0;
  pRenderClient->lpVtbl->ReleaseBuffer(pRenderClient, FramesN, 0);
  
  while (1) {
    UINT padding;
    
    pClient->lpVtbl->GetCurrentPadding(pClient, &padding);
    int writeframesn = FramesN - padding;
    if (writeframesn > 0) {
      pRenderClient->lpVtbl->GetBuffer(pRenderClient, writeframesn, &frames);
      
      for (int i = 0; i < writeframesn; i++)
	      frames[i] = 0;
	    
	    pRenderClient->lpVtbl->ReleaseBuffer(pRenderClient, writeframesn, 0);
	  }
    
    Sleep(12);
  }
  WAVE_free();
  return 0;
}

