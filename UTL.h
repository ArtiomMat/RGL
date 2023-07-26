#pragma once

// I yoinked it from an old AVE version.

typedef unsigned long long ULONGLONG;
typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

typedef struct {
	ULONGLONG* Data;
	UINT N, MemN;
} UTL_LLIST;

enum {
	UTL_MBYesNo,
	UTL_MBYesNoCancel,
	UTL_MBOk,
	UTL_MBOkCancel,
};

typedef struct {
	void* Next;
	void* Prev;
} UTL_LINK;

typedef struct {
	UCHAR* Data;
	UINT Size;
	USHORT WCur, RCur;
} UTL_CBUF;

/*
	The usual formula is Compared-Other.
	If returns >0, list assumes Compared should come before Other.
	If returns <0, list assumes Other should come before Compared.
	If returns 0, list leaves Compared and Other as they were.
*/
// typedef int (*UTL_ListCmp_t) (ULONGLONG Compared, ULONGLONG Other);
// typedef int (*UTL_LinkCmp_t) (void* Next, ULONGLONG Other);

// UTL_Init() please.
extern char* UTL_exePath;
extern char UTL_isBigEndian;

void UTL_Init();
// NOTE: Not thread safe. Needs UTL_Init.
char* UTL_RelPath(char* Str);
void UTL_Free();

void UTL_SetBuf(UTL_CBUF* B, void* Data, int Size);
void UTL_InitBuf(UTL_CBUF* B, int Size);
void UTL_FreeBuf(UTL_CBUF* B);
void UTL_ResetBuf(UTL_CBUF* B);
// Returns overflow, <=0 means you are ok.
int UTL_Read(UTL_CBUF* B, UCHAR** DataPtr, int N);
// Return is same as UTL_Read
int UTL_Copy(UTL_CBUF* B, UCHAR* Data, int N);
// Returns overflow, <=0 means you are ok.
int UTL_Write(UTL_CBUF* B, UCHAR* Data, int N);
// Returns same as UTL_Write
// Simulate a write and check if we overflown, a simple arithmetic calculation.
int UTL_SimWrite(UTL_CBUF* B, int N);
// Returns same as UTL_Read
// Simulate a read and check if we overflown, a simple arithmetic calculation.
int UTL_SimRead(UTL_CBUF* B, int N);

void UTL_InitLink(void* Link);
// Sorts the entire linked list
void UTL_SortLink(void* Link);
// While this is O(n), if you were to use it for every link at once, you essentially get O(n^2).
// What you want is to use it if links are not added frequently, otherwise if a bunch of links are added at once you are better off first loading them all in, and then you can SortLink()
void UTL_SortAddLink(void* Link);
void UTL_AddLink(void* Added, void* Next);
void UTL_RemLink(void* Removed);

void UTL_InitList(UTL_LLIST* List);
void UTL_FreeList(UTL_LLIST* List);
void UTL_AddToList(UTL_LLIST* List, ULONGLONG X);
// If Cmp is 0 then we just do integer comparison.
// void UTL_SortList(UTL_LLIST* List, UTL_ListCmp_t Cmp);
// DONT UTL_FreeList AFTER UTILIZING THIS.
// Returns an optimized array that is exactly the size needed to store the list.
// This doesn't copy the list to an array, it literally devolves it.
ULONGLONG* UTL_DevolveList(UTL_LLIST* List);

void UTL_MemReverse(UCHAR* Mem, int N);
void UTL_SwapEndian64(ULONGLONG* X);
void UTL_SwapEndian32(UINT* X);
void UTL_SwapEndian16(USHORT* X);

// TODO: Maybe make them inline, it's extremly simple operations.
void UTL_ILilE64(ULONGLONG* X);
void UTL_ILilE32(UINT* X);
void UTL_ILilE16(USHORT* X);
void UTL_IBigE64(ULONGLONG* X);
void UTL_IBigE32(UINT* X);
void UTL_IBigE16(USHORT* X);

ULONGLONG UTL_OLilE64(ULONGLONG X);
UINT UTL_OLilE32(UINT X);
USHORT UTL_OLilE16(USHORT X);
ULONGLONG UTL_OBigE64(ULONGLONG X);
UINT UTL_OBigE32(UINT X);
USHORT UTL_OBigE16(USHORT X);

// 0 = no, 1 = yes, 2 = ok, 3 = cancel
int UTL_MessageBox(const char* Title, const char* Desc, int Type);

int UTL_FileExits(const char* FP);
