/*
  ***************************************************
  PSPdisp (c) 2008 Jochen Schleu

  pspdisp.h - global defines

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef PSPDISP_H
#define PSPDISP_H



#include <stdarg.h>
#include <stddef.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>



#define ALLOC_TAG 'bfDD'

// if defined, outputs debug information for a kernel debugger
// e.g. DebugView
#define KERNEL_DEBUGGER_OUTPUT

typedef struct _PDEV
{
  HANDLE  hDriver;  // handle to the driver
  HDEV    hdevEng;  // handle to device
  HSURF   hsurfEng; // handle to the generated surface
  DHSURF  dhsurfEng;
  HANDLE  dllHandle;// handle to the loaded kernel mode dll
  int     width;    // width of current display mode
  int     height;
} PDEV, *PPDEV;


typedef struct
{
  int width;
  int height;
} displayMode;


// number of supported resolutions
#define MODE_COUNT 21

// supported resolutions
static displayMode displayModes[] =
{
  { 480, 272 },
  { 560, 315 },
  { 640, 360 },
  { 720, 405 },
  { 800, 450 },
  { 640, 480 },
  { 800, 480 },
  { 854, 480 },
  { 880, 495 },
  { 960, 544 },
  { 800, 600 },
  { 1024, 576 },
  { 1024, 768 },
  { 1280, 720 },
  { 1280, 800 },
  { 1280, 960 },
  { 1280, 1024 },
  { 1400, 1050 },
  { 1680, 1050 },
  { 1920, 1080 },
  { 1600, 1200 }
};


// Variables for the DrvEscape routine
#define ESCAPE_IS_DIRTY (0x10000 + 100)

unsigned int screenIsDirty;


// Default resolution
#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 544


HSURF bitmapSurface;

BOOL fillGDIINFO(OUT GDIINFO *pdevcaps, IN DEVMODEW *pdm, OUT PPDEV ppdev);
BOOL fillDEVINFO(OUT DEVINFO *pdi, IN DEVMODEW *pdm);

#endif