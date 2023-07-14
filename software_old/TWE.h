#pragma once

// TypeWriter Emulator

#define TWE_MAXINSIZE 72

#define TWE_DEFAULTFONT "def.twf"

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int UINT;

typedef struct {
  USHORT code;
  UCHAR down;
} TWE_IN;

enum {
  TWE_INULL=0,

  TWE_IENTER='\n',
  TWE_IESCAPE='\e',
  TWE_IBACK='\b',
  TWE_ITAB='\t',
  TWE_ICONTROL=129,
  TWE_IMETA,
  TWE_ISHIFT,
  TWE_ICAPS,

  TWE_ILMOUSE=500,
  TWE_IRMOUSE,
  TWE_IMMOUSE,

  TWE_IEXIT,
};

// The mouse's position in character dimentions.
extern UINT TWE_mcol, TWE_mrow;
// Raw pixel data
extern UCHAR* TWE_pixels;
extern UCHAR TWE_colors[256][3];
// This queue includes keyboard/mouse button states(up/down) as of now.
// You loop through it, until you hit code=0, which means that there are no more inputs.
extern TWE_IN TWE_inputs[TWE_MAXINSIZE];

// By default 0.
extern UCHAR TWE_nullcolorindex;

// The position of the cursor which stamps and erases and shit
extern UINT TWE_col, TWE_row;

void TWE_init(UCHAR _scale, UINT cols, UINT rows);
void TWE_free();

// Contains:
// char number of palette indices
// char[0 to 256][3] of RGB triplets
//
void TWE_loadfont(const char* fp);

// Fills a cell with a color index.
// This also renders the null color.
void TWE_fill(UCHAR ci);
// Stamps a character on a cell, omg infinite layers!
// If the null color is found nothing is rendered in that pixel.
void TWE_stamp(UINT ch);
// Erases a cell and puts TWE_nullcolorindex
void TWE_erase();

// Monochrome stamp
// Uses TWE_nullcolorindex and if it finds this color then we put bgi, if we find any other index in the character we put fgi.
void TWE_monos(UINT ch, UCHAR fgi, UCHAR bgi);

// Print all the changes on the window.
// This additionally overrides the input queue with new input.
void TWE_print();

