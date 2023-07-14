#include "TWE.h"

#include <stdio.h>
#include <stdint.h>

#include <windows.h>

// Quake, thank you.
typedef struct {
	BITMAPINFOHEADER header;
	RGBQUAD colors[256];
} DIBINFO;

// Note that this is not a full struct, just the character part, colors are outside.
typedef struct {
  uint32_t n;
  uint8_t w, h;
  // We can page through the characters by just +=w*h
  UCHAR* pixels;
} FONT;

UINT TWE_mcol, TWE_mrow;
UCHAR* TWE_pixels;
UCHAR TWE_colors[256][3];
TWE_IN TWE_inputs[TWE_MAXINSIZE];

UCHAR TWE_nullcolorindex = 0;

static UCHAR scale;

UINT TWE_col, TWE_row;

static int inputi = 0;

static UINT width, height;

static FONT loadedfont;

static HWND hWnd = 0;
static HDC hDC;

static HBITMAP hDIB;
static HDC hDIBDC;

static int input(USHORT code, UCHAR down) {
  if (inputi >= TWE_MAXINSIZE)
    return 0;

  TWE_inputs[inputi] = (TWE_IN){.code=code, .down=down};
  
  inputi++;
  
  return 1;
}

static UINT convertkey(UINT k) {
  switch (k) {
    case VK_BACK:
    return TWE_IBACK;
    case VK_RETURN:
    return TWE_IENTER;
    case VK_TAB:
    return TWE_ITAB;
    case VK_ESCAPE:
    return TWE_IESCAPE;
    case VK_MENU:
    return TWE_IMETA;
    case VK_CONTROL:
    return TWE_ICONTROL;
    case VK_SHIFT:
    return TWE_ISHIFT;
    case VK_CAPITAL:
    return TWE_ICAPS;
    default:
    return k;
  }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		// Using this for Wine compatability. other shit wont work.
		case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
			input(TWE_IEXIT, 1);
		else
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;

		case WM_MOUSEMOVE:
		{
			// Extract the mouse coordinates from the lParam parameter
			TWE_mcol = LOWORD(lParam)/loadedfont.w;
      TWE_mcol /= scale;
			TWE_mrow = HIWORD(lParam)/loadedfont.h;
      TWE_mrow /= scale;
			break;
		}

    case WM_KEYDOWN:
    case WM_KEYUP:
    UINT k = convertkey(wParam);
    input(wParam, 1-(uMsg-WM_KEYDOWN));
    break;

		case WM_LBUTTONDOWN:
		input(TWE_ILMOUSE, 1);
		break;
    
    case WM_LBUTTONUP:
		input(TWE_ILMOUSE, 0);
		break;

		default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

static void applypal() {
  static RGBQUAD rgbs[256];

  // Copy
  for (int i = 0; i < 256; i++) {
    rgbs[i].rgbRed = TWE_colors[i][0];
    rgbs[i].rgbGreen = TWE_colors[i][1];
    rgbs[i].rgbBlue = TWE_colors[i][2];
    rgbs[i].rgbReserved = 0; // Must be 0
  } 

	if (!SetDIBColorTable(hDIBDC, 0, 256, (const RGBQUAD *)rgbs))
		(void)puts("TWE: Failed to apply palette.");
}

// TODO: Make it endian friendly. for now only x86 can really read it.
void TWE_loadfont(const char* fp) {
  FILE* f = fopen(fp, "rb");

  // Character data
  fread(&loadedfont.n, sizeof(loadedfont.n), 1, f);
  fread(&loadedfont.w, sizeof(loadedfont.w), 1, f);
  fread(&loadedfont.h, sizeof(loadedfont.h), 1, f);

  // color palette
  for (int i = 0; i < 256; i++)
    for (int j = 0; j < 3; j++)
      TWE_colors[i][j] = fgetc(f);
 
  if (hWnd) // May be before we created the window
    applypal();
  
  // Load the characters
  loadedfont.pixels = realloc(loadedfont.pixels, sizeof(*loadedfont.pixels) * loadedfont.n * loadedfont.w * loadedfont.h);
  fread(loadedfont.pixels, sizeof(*loadedfont.pixels), loadedfont.n * loadedfont.w * loadedfont.h, f);
}

void TWE_init(UCHAR _scale, UINT cols, UINT rows) {
  // Reset shit
  loadedfont.pixels = NULL;
  TWE_col = TWE_row = inputi = TWE_inputs[0].code = 0;

  TWE_loadfont(TWE_DEFAULTFONT);

  scale = _scale;
  width = cols*loadedfont.w;
  height = rows*loadedfont.h;

  // Create the class
  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = "TWEWINDOW";
  if (!RegisterClassEx(&wc)) {
    (void)puts("TWE: Failed to create class!");
    return;
  }

  // Adjusting the window size
  // Thank you so much Id for making Quake open source.
  RECT WndRect;
  int TrueW, TrueH;
  
  WndRect.left = 0;
  WndRect.top = 0;
  WndRect.right = width*scale;
  WndRect.bottom = height*scale;
  
  DWORD dwStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX;
  
  (void)AdjustWindowRect (&WndRect, dwStyle, 0);
  TrueW = WndRect.right - WndRect.left;
  TrueH = WndRect.bottom - WndRect.top;

  hWnd = CreateWindowEx(0, "TWEWINDOW", NULL,
    dwStyle,
    CW_USEDEFAULT, CW_USEDEFAULT,
    TrueW, TrueH,
    NULL, NULL, GetModuleHandle(NULL), NULL);
  if (!hWnd) {
    (void)puts("TWE: Failed to create window!");
    return;
  }
  
  hDC = GetDC(hWnd);
  
  DIBINFO di = {0};
  di.header.biSize          = sizeof(BITMAPINFOHEADER);
  di.header.biWidth         = width; // Not multiplied by scale because it is the bitamp
  di.header.biHeight        = -height; // top left corner is 0,0
  di.header.biPlanes        = 1;
  di.header.biBitCount      = 8;
  di.header.biCompression   = BI_RGB;
  di.header.biSizeImage     = 0;
  di.header.biXPelsPerMeter = 0;
  di.header.biYPelsPerMeter = 0;
  di.header.biClrUsed       = 255;
  di.header.biClrImportant  = 255;

  if (!( hDIB = CreateDIBSection( hDC, (BITMAPINFO*)&di, DIB_RGB_COLORS, (void**)&TWE_pixels, NULL, 0) )) {
    (void)puts("TWE: Failed to create DIB!");
    return;
  }
  
  if (!( hDIBDC = CreateCompatibleDC( hDC ) )) {
    (void)puts("TWE: Failed to create compatible DC!");
    return;
  }

  (void)SelectObject( hDIBDC, hDIB );
  applypal();
}

void TWE_free() {
  (void)DeleteObject(hDIB);
	(void)DeleteDC(hDIBDC);
	(void)ReleaseDC(hWnd, hDC);
	(void)DestroyWindow(hWnd);
	(void)UnregisterClassA("TWEWINDOW", GetModuleHandle(NULL));

  (void)free(loadedfont.pixels);
}

void TWE_stamp(UINT ch) {  
  UINT starty = TWE_row*loadedfont.h, startx = TWE_col*loadedfont.w;
  UCHAR* data = loadedfont.pixels + loadedfont.h*loadedfont.w*ch;
  
  for (UINT dy = 0, y = starty; dy < loadedfont.h; y++, dy++)
    for (UINT dx = 0, x = startx; dx < loadedfont.w; x++, dx++)
      if (data[dy*loadedfont.w + dx] != TWE_nullcolorindex)
        TWE_pixels[y*width+x] = data[dy*loadedfont.w + dx];
}

void TWE_monos(UINT ch, UCHAR fgi, UCHAR bgi) {  
  UINT starty = TWE_row*loadedfont.h, startx = TWE_col*loadedfont.w;
  UCHAR* data = loadedfont.pixels + loadedfont.h*loadedfont.w*ch;
  
  for (UINT dy = 0, y = starty; dy < loadedfont.h; y++, dy++)
    for (UINT dx = 0, x = startx; dx < loadedfont.w; x++, dx++)
      if (data[dy*loadedfont.w + dx] == TWE_nullcolorindex)
        TWE_pixels[y*width+x] = bgi;
      else
        TWE_pixels[y*width+x] = fgi;
}

void TWE_fill(UCHAR ci) {
  UINT starty = TWE_row*loadedfont.h, startx = TWE_col*loadedfont.w;
  for (UINT y = starty; y < starty+loadedfont.h; y++)
    for (UINT x = startx; x < startx+loadedfont.w; x++)
      TWE_pixels[y*width+x] = ci;
}

void TWE_erase() {
  TWE_fill(TWE_nullcolorindex);
}

void TWE_print() {
  static MSG Msg;

  inputi = 0; // Reset input

	// Message pipe
	while(PeekMessage(&(Msg), NULL, 0, 0, PM_REMOVE)) {
		(void)TranslateMessage(&(Msg));
		(void)DispatchMessageA(&(Msg));
	}

  input(0, 0); // Add the null terminator

	// (void)StretchBlt(hDC, 0, 0, width, height, hDIBDC, 0, 0, SRCCOPY);
  (void)StretchBlt(hDC, 0, 0, width*scale, height*scale, hDIBDC, 0, 0, width, height, SRCCOPY);
}

static int _main() {
  TWE_init(1,64,32);
  TWE_stamp('H');
  
  for (int i = 0; i < 16; i++) {
    TWE_col = i;
    TWE_fill(i);
  }

  while (1) {
    TWE_print();
    
    for(TWE_IN *i = TWE_inputs; i->code; i++) {
      if (i->code == TWE_IEXIT)
        exit(0);
      else if (i->code == TWE_ILMOUSE) {
        if (i->down == 0) {
          TWE_col = TWE_mcol;
          TWE_row = TWE_mrow;
          TWE_fill(0);
        }
      }
      else if (i->down ==1) {
        printf("%c", i->code);
        TWE_monos(i->code, 2, 0);
        TWE_col++;
      }
    }

    Sleep(21);
  }
  return 0;
}
