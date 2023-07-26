#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <shlwapi.h>

#include "UTL.h"

#define PathLength 512

static char Path[PathLength];
static int ExePathEndI;

char* UTL_exePath;
char UTL_isBigEndian;

void UTL_Init() {
	char TmpExePath[PathLength];
	GetModuleFileName(NULL, TmpExePath, PathLength);
	// Size is the total length of TmpExePath + NULL terminator
	int Size;
	
	// We be multitasking, we copy the path, remove game.exe, find the ExePathEndI of UTL_exePath
	for (Size = 0; TmpExePath[Size]; Size++) {
		char C = TmpExePath[Size];
		Path[Size] = TmpExePath[Size];
		
		if (C == '\\')
			ExePathEndI = Size;
	}
	TmpExePath[ExePathEndI+1] = 0;

	UTL_exePath = malloc(ExePathEndI+1);
	strcpy(UTL_exePath, TmpExePath);

	// Check endian
	union {
		USHORT I;
		UCHAR C[2];
	} U;
	U.I = 1;

	UTL_isBigEndian = (U.C[1] == 1);
	printf("UTL: Utility initialized. System is %s endian.\n", UTL_isBigEndian?"big":"small");


	// TODO: It searches the path(skipping the EXE's path, and checks if a DLL exists)
	// Use this as a base to delete the DLLs if the user has them already.
	/*
		char libraryName[] = "mylibrary.dll";
	char libraryPath[MAX_PATH];

	// Get the path of the current module (EXE file)
	char modulePath[MAX_PATH];
	GetModuleFileNameA(NULL, modulePath, MAX_PATH);

	// Extract the directory path from the module path
	char directoryPath[MAX_PATH];
	strncpy(directoryPath, modulePath, strrchr(modulePath, '\\') - modulePath);
	directoryPath[strrchr(modulePath, '\\') - modulePath] = '\0';

	// Search for the library in the system's PATH, excluding the EXE's directory
	DWORD result = SearchPathA(directoryPath, libraryName, NULL, MAX_PATH, libraryPath, NULL);
	if (result != 0) {
		printf("Library found at path: %s\n", libraryPath);
	} else {
		printf("Library not found in the system's PATH (excluding the EXE's directory).\n");
	}
	*/
}

char* UTL_RelPath(char* Str) {
	int PathI, StrI;
	for (PathI = ExePathEndI+1, StrI = 0; Str[StrI] && PathI < PathLength; PathI++, StrI++)
		Path[PathI] = Str[StrI];
	
	Path[PathI] = 0;

	return Path;
}

void UTL_Free() {
	free(UTL_exePath);
}

void UTL_SetBuf(UTL_CBUF* B, void* Data, int Size) {
	B->Data = Data;
	B->Size = Size;
	B->RCur = B->WCur = 0;
}
void UTL_InitBuf(UTL_CBUF* B, int Size) {
	B->Data = malloc(Size);
	B->Size = Size;
	B->RCur = B->WCur = 0;
}
void UTL_FreeBuf(UTL_CBUF* B) {
	free(B->Data);
}
void UTL_ResetBuf(UTL_CBUF* B) {
	B->RCur = B->WCur = 0;
}
int UTL_Read(UTL_CBUF* B, UCHAR** DataPtr, int N) {
	*DataPtr = B->Data + B->RCur;
	B->RCur += N;
	return B->RCur - B->Size;
}
int UTL_Copy(UTL_CBUF* B, UCHAR* Data, int N) {
	for (int I = 0; B->RCur < B->Size && I < N; I++, B->RCur++)
		Data[I] = B->Data[B->RCur];
	
	return UTL_SimRead(B, N);
}
int UTL_Write(UTL_CBUF* B, UCHAR* Data, int N) {
	for (int I = 0; B->WCur < B->Size && I < N; I++, B->WCur++)
		Data[I] = B->Data[B->WCur];
	
	return UTL_SimWrite(B, N);
}

int UTL_FileExits(const char* FP) {
	return PathFileExistsA(FP);
}

// Simulate a write and check if we overflown, a simple arithmetic calculation.
int UTL_SimWrite(UTL_CBUF* B, int N) {
	return B->WCur + N - B->Size;
}
// Simulate a read and check if we overflown, a simple arithmetic calculation.
int UTL_SimRead(UTL_CBUF* B, int N) {
	return B->RCur + N - B->Size;
}

int UTL_MessageBox(const char* Title, const char* Desc, int Type) {
	UINT uType;
	switch (Type) {
		case UTL_MBYesNoCancel:
		uType = MB_YESNOCANCEL;
		break;
		case UTL_MBOk:
		uType = MB_OK;
		break;
		case UTL_MBOkCancel:
		uType = MB_OKCANCEL;
		break;
		default:
		uType = MB_YESNO;
		break;
	}
	int Answer = MessageBox(NULL, Desc, Title, uType);
	
	switch (Answer) {
		case IDOK:
		return 2;
		case IDYES:
		return 1;
		case IDNO:
		return 0;
		default:
		return 3;
	}
}

void UTL_InitLink(void* _Link) {
	UTL_LINK* Link = _Link;
	Link->Next = Link->Prev = NULL;
}

void UTL_AddLink(void* Added, void* To) {
	if (To)
		((UTL_LINK*)To)->Prev = Added;
	if (Added)
		((UTL_LINK*)Added)->Next = To;
}
void UTL_RemLink(void* Removed) {
	if (Removed) {
		UTL_LINK* Next = ((UTL_LINK*)Removed)->Next;
		UTL_LINK* Prev = ((UTL_LINK*)Removed)->Prev;
		if (Prev)
			Prev->Next = Next;
		if (Next)
			Next->Prev = Prev;
	}
}

// static int DefListCmp(ULONGLONG Compared, ULONGLONG Other) {
// 	if (Compared > Other)
// 		return 1;
// 	else if (Compared < Other)
// 		return -1;
// 	return 0;
// }

void UTL_InitList(UTL_LLIST* List) {
	List->N = 0;
	List->MemN = 2;
	List->Data = malloc(List->MemN * sizeof(List->Data));
}
void UTL_FreeList(UTL_LLIST* List) {
	free(List->Data);
}
void UTL_AddToList(UTL_LLIST* List, ULONGLONG X) {
	if (List->N >= List->MemN) {
		List->MemN *= 2;
		List->Data = realloc(List->Data, List->MemN * sizeof(List->Data));
	}

	List->Data[List->N] = X;

	List->N++;
}

ULONGLONG* UTL_DevolveList(UTL_LLIST* List) {
	// Resize Data to be of size N*ULONGLONG
	if (List->MemN > List->N)
		List->Data = realloc(List->Data, List->N * sizeof(List->Data));
	
	ULONGLONG* Ret = List->Data;
	List->Data = NULL; // Incase the dev doesn't listen to the fat warning we nullify Data so UTL_FreeList doesn't free the returned data

	return Ret;
}

// ======================================
// ENDIAN SHIT
// ======================================

void UTL_MemReverse(UCHAR* Mem, int N) {
	for (int I = 0; I < N/2; I++) {
		UCHAR Tmp = Mem[I];
		Mem[I] = Mem[N-1-I];
		Mem[N-1-I] = Tmp;
	}
}

void UTL_SwapEndian64(ULONGLONG* X) {
	*X = ((*X & 0xFF00000000000000) >> 56) |
		   ((*X & 0x00FF000000000000) >> 40) |
		   ((*X & 0x0000FF0000000000) >> 24) |
		   ((*X & 0x000000FF00000000) >> 8) |
		   ((*X & 0x00000000FF000000) << 8) |
		   ((*X & 0x0000000000FF0000) << 24) |
		   ((*X & 0x000000000000FF00) << 40) |
		   ((*X & 0x00000000000000FF) << 56);
}

void UTL_SwapEndian32(UINT* X) {
	*X = ((*X & 0xFF000000) >> 24) |
		 ((*X & 0x00FF0000) >> 8) |
		 ((*X & 0x0000FF00) << 8) |
		 ((*X & 0x000000FF) << 24);
}

void UTL_SwapEndian16(USHORT* X) {
	*X = ((*X & 0xFF00) >> 8) |
		   ((*X & 0x00FF) << 8);
}

// Input endianess stuff
void UTL_ILilE64(ULONGLONG* X) {
	if (UTL_isBigEndian)
		UTL_SwapEndian64(X);
}
void UTL_ILilE32(UINT* X) {
	if (UTL_isBigEndian)
		UTL_SwapEndian32(X);
}
void UTL_ILilE16(USHORT* X) {
	if (UTL_isBigEndian)
		UTL_SwapEndian16(X);
}

void UTL_IBigE64(ULONGLONG* X) {
	if (!UTL_isBigEndian)
		UTL_SwapEndian64(X);
}
void UTL_IBigE32(UINT* X) {
	if (!UTL_isBigEndian)
		UTL_SwapEndian32(X);
}
void UTL_IBigE16(USHORT* X) {
	if (!UTL_isBigEndian)
		UTL_SwapEndian16(X);
}

// Output endianess stuff
ULONGLONG UTL_OLilE64(ULONGLONG X) {
	if (!UTL_isBigEndian)
		UTL_SwapEndian64(&X);
	return X;
}
UINT UTL_OLilE32(UINT X) {
	if (!UTL_isBigEndian)
		UTL_SwapEndian32(&X);
	return X;
}
USHORT UTL_OLilE16(USHORT X) {
	if (!UTL_isBigEndian)
		UTL_SwapEndian16(&X);
	return X;
}

ULONGLONG UTL_OBigE64(ULONGLONG X) {
	if (UTL_isBigEndian)
		UTL_SwapEndian64(&X);
	return X;
}
UINT UTL_OBigE32(UINT X) {
	if (UTL_isBigEndian)
		UTL_SwapEndian32(&X);
	return X;
}
USHORT UTL_OBigE16(USHORT X) {
	if (UTL_isBigEndian)
		UTL_SwapEndian16(&X);
	return X;
}
