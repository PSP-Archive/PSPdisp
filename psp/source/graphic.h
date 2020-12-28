/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  graphic.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include "common.h"
#include "debug.h"
#include "menu.h"
#include "shared.h"

// Intrafont library
#include "library/intrafont.h"

// Danzeff keyboard
#include "library/danzeff.h"


// Rotation of the transmited image
unsigned int rotation;
unsigned int lastRotation;

// Intrafont data
intraFont* sansSerifSmall;
intraFont* sansSerifLarge;


unsigned int __attribute__((aligned(16))) list[262144];

void* l_frameBuffer;

#define COLOR_RED 0xFF0000FF
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_LITEGRAY 0xFFBFBFBF
#define COLOR_GRAY 0xFF7F7F7F
#define COLOR_DARKGRAY 0xFF3F3F3F
#define COLOR_BLACK 0x88000000

#define ROTATION_OFF 0
#define ROTATION_90 1
#define ROTATION_180 2
#define ROTATION_270 3



typedef struct
{
  float u, v;
  unsigned int color;
  float x,y,z;
}
texturedVertex;


typedef struct
{
  unsigned int color;
  float x,y,z;
}
coloredVertex;


typedef struct
{
  float x,y,z;
}
plainVertex;


void graphicInit();
void graphicInitFontLibrary();
void graphicSwitchBuffers();
void graphicClearScreen(unsigned int color);
void graphicDrawBackground(bool drawBars, bool transparent);
void graphicSetupOrtho();
void graphicDrawTexturedRectangle(void* texture, int startX, int startY, int width, int height);
void graphicFillPlainVertex(plainVertex* vertex, int x, int y, int z);
void graphicFillColoredVertex(coloredVertex* vertex, int color, int x, int y, int z);
void graphicFillTexturedVertex(texturedVertex* vertex, int color, int u, int v, int x, int y, int z);
void graphicDrawColoredRectangle(int colorTop, int colorBottom, int startX, int startY, int width, int height);
void graphicDrawTexturedRectangle(void* texture, int startX, int startY, int width, int height);
void graphicSlicedBlit(float startX, float startY, float imageWidth, float imageHeight, float textureWidth, float textureHeight);
void graphicRedrawLastFrame();
void graphicDrawFrame(unsigned int* textureData, unsigned int rotation, bool forceDrawing);
void graphicDrawWave(float period, float offset, float amplitude);
void graphicStartDrawing();
void graphicFinishDrawing(bool switchBuffer);
void graphicDrawDanzeff();


#endif