#include <windows.h>
#include "TM.h"

// THE MOTHER FUCKER HID THIS FROM US:
NTSTATUS NtQueryTimerResolution(
	OUT PULONG MinimumResolution, 
	OUT PULONG MaximumResolution, 
	OUT PULONG ActualResolution
);
NTSTATUS NtSetTimerResolution(
	IN ULONG RequestedResolution, 
	IN BOOLEAN Set, 
	OUT PULONG ActualResolution
);

static ULONG TM_GameMSPF = 1000/12;
static ULONG LastFrameTime;

ULONG TM_init(UCHAR GameFPS) {
	TM_GameMSPF = 1000/GameFPS;

	ULONG MinSleepRes, DefaultSleepRes;
	// Ticks are measuered in 100 nanoseconds
	NtQueryTimerResolution(&MinSleepRes, &DefaultSleepRes, &DefaultSleepRes);
	DefaultSleepRes /= 10000;

	return DefaultSleepRes;
}

ULONG TM_now() {
	LARGE_INTEGER Freq, Counter;
	QueryPerformanceFrequency(&Freq);
	QueryPerformanceCounter(&Counter);

	return (Counter.QuadPart * 1000) / Freq.QuadPart;
}

void TM_sleep(ULONG X) {
	Sleep(X);
}

void TM_initwait() {
	LastFrameTime = TM_now();
}

void TM_wait() {
	static int SkipN = 0;
  
  if (SkipN) {
    SkipN--;
    return;
  }

	ULONG Delta = (ULONG)(TM_now() - LastFrameTime);
	LONG WaitTime = TM_GameMSPF - Delta;
	if (WaitTime > 0) {
		Sleep(WaitTime);
	}
	else {
		SkipN += (-WaitTime)/TM_GameMSPF;
	}

	LastFrameTime = TM_now();
}
