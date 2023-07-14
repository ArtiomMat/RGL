#pragma once

// Time utility library for windows

typedef unsigned char UCHAR;
typedef unsigned long ULONG;

ULONG TM_init(UCHAR GameFPS);
ULONG TM_now();
void TM_sleep(ULONG X);
void TM_initwait();
// Returns how many frames to skip
// Call initwait first
void TM_wait();
