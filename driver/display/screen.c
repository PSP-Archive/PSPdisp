/*
  ***************************************************
  PSPdisp (c) 2008 Jochen Schleu

  screen.c - defining the screen surface

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "debug.h"
#include "pspdisp.h"


// Fill the GDIINFO structure.
BOOL fillGDIINFO(OUT GDIINFO *pdevcaps, IN DEVMODEW *pdm, OUT PPDEV ppdev)
{
  ULONG pelsWidth = pdm->dmPelsWidth;
  ULONG pelsHeight = pdm->dmPelsHeight;

  DebugPrint("fillGDIINFO - mode requested = %d x %d x %d @ %d Hz\n", pdm->dmPelsWidth, pdm->dmPelsHeight, pdm->dmBitsPerPel, pdm->dmDisplayFrequency);

  // Check what mode was requested.
  if (((pdm->dmPelsWidth == 0) && (pdm->dmPelsHeight == 0) && (pdm->dmBitsPerPel == 0) && (pdm->dmDisplayFrequency == 0))) {
    // It's the request for the default mode
  	pelsWidth = DISPLAY_WIDTH;
	  pelsHeight = DISPLAY_HEIGHT;
  }

  // Save the chosen resolution in the pdev.
  ppdev->width = pelsWidth;
  ppdev->height = pelsHeight;

  // Fill in the GDIINFO.
  memset(pdevcaps, 0, sizeof(GDIINFO));
  
  /*ULONG*/   pdevcaps->ulVersion = 0x5000; // for Win2k
  /*ULONG*/   pdevcaps->ulTechnology = DT_RASDISPLAY;
  /*ULONG*/   pdevcaps->ulHorzSize = pelsWidth;
  /*ULONG*/   pdevcaps->ulVertSize = pelsHeight;
  /*ULONG*/   pdevcaps->ulHorzRes = pelsWidth;
  /*ULONG*/   pdevcaps->ulVertRes = pelsHeight;
  /*ULONG*/   pdevcaps->cBitsPixel = 32; // bits per pixel
  /*ULONG*/   pdevcaps->cPlanes = 1; // must be 1!
  /*ULONG*/   pdevcaps->ulNumColors = (ULONG)(-1); // no palette
  /*ULONG*/   pdevcaps->flRaster = 0; // reserved
  /*ULONG*/   pdevcaps->ulLogPixelsX = 96; // must be 96 for displays
  /*ULONG*/   pdevcaps->ulLogPixelsY = 96;
  /*ULONG*/   pdevcaps->flTextCaps = TC_RA_ABLE; // the framebuffer example sets this, not sure
  /*ULONG*/   pdevcaps->ulDACRed = 8; // red bits per pixel
  /*ULONG*/   pdevcaps->ulDACGreen = 8;
  /*ULONG*/   pdevcaps->ulDACBlue = 8;
  /*ULONG*/   pdevcaps->ulAspectX = 0x24; // we have square pixels
  /*ULONG*/   pdevcaps->ulAspectY = 0x24;
  /*ULONG*/   pdevcaps->ulAspectXY = 0x33; // sqrt(ulAspectX^2 + ulAspectY^2)
  /*LONG*/    pdevcaps->xStyleStep = 1; // steps for diagonal line drawing (from SDK)
  /*LONG*/    pdevcaps->yStyleStep = 1;
  /*LONG*/    pdevcaps->denStyleStep = 3;
  /*POINTL*/  pdevcaps->ptlPhysOffset.x = 0; // no offset
              pdevcaps->ptlPhysOffset.y = 0;
  /*SIZEL*/   pdevcaps->szlPhysSize.cx = pelsWidth; // display width
              pdevcaps->szlPhysSize.cy = pelsHeight; // display height
  /*ULONG*/   pdevcaps->ulNumPalReg = 0; // no palette
  /*COLORINFO*/ 
              pdevcaps->ciDevice.Red.x = 6700; // color in CIE coordinate space, just copied from SDK
              pdevcaps->ciDevice.Red.y = 3300;
              pdevcaps->ciDevice.Red.Y = 0;
              pdevcaps->ciDevice.Green.x = 2100;
              pdevcaps->ciDevice.Green.y = 7100;
              pdevcaps->ciDevice.Green.Y = 0;
              pdevcaps->ciDevice.Blue.x = 1400;
              pdevcaps->ciDevice.Blue.y = 800;
              pdevcaps->ciDevice.Blue.Y = 0;
              pdevcaps->ciDevice.AlignmentWhite.x = 3127;
              pdevcaps->ciDevice.AlignmentWhite.y = 3290;
              pdevcaps->ciDevice.AlignmentWhite.Y = 0;
              pdevcaps->ciDevice.RedGamma = 20000;
              pdevcaps->ciDevice.GreenGamma = 20000;
              pdevcaps->ciDevice.BlueGamma = 20000;
              pdevcaps->ciDevice.Cyan.x = 0;
              pdevcaps->ciDevice.Cyan.y = 0;
              pdevcaps->ciDevice.Cyan.Y = 0;
              pdevcaps->ciDevice.Magenta.x = 0;
              pdevcaps->ciDevice.Magenta.y = 0;
              pdevcaps->ciDevice.Magenta.Y = 0;
              pdevcaps->ciDevice.Yellow.x = 0;
              pdevcaps->ciDevice.Yellow.y = 0;
              pdevcaps->ciDevice.Yellow.Y = 0;
              pdevcaps->ciDevice.MagentaInCyanDye = 0;
              pdevcaps->ciDevice.YellowInCyanDye = 0;
              pdevcaps->ciDevice.CyanInMagentaDye = 0;
              pdevcaps->ciDevice.YellowInMagentaDye = 0;
              pdevcaps->ciDevice.CyanInYellowDye = 0;
              pdevcaps->ciDevice.MagentaInYellowDye = 0;

  /*ULONG*/   pdevcaps->ulDevicePelsDPI = 0; // always 0 for displays
  /*ULONG*/   pdevcaps->ulPrimaryOrder = PRIMARY_ORDER_ABC; // color order is RGB
  /*ULONG*/   pdevcaps->ulHTPatternSize = HT_PATSIZE_4x4_M; // I don't know, SDK samples use this
  /*ULONG*/   pdevcaps->ulHTOutputFormat = HT_FORMAT_32BPP;
  /*ULONG*/   pdevcaps->flHTFlags = HT_FLAG_ADDITIVE_PRIMS; // only one that applies
  /*ULONG*/   pdevcaps->ulVRefresh = 60; // not important

  // The rest of the GDIINFO members can be ignored.

  DebugPrint("fillGDIINFO - exit\n");

  return TRUE;
}




BOOL fillDEVINFO(OUT DEVINFO *pdi, IN DEVMODEW *pdm)
{

  LOGFONTW defaultFont = {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"System"};
  LOGFONTW ansiVarFont = {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"};
  LOGFONTW ansiFixFont = {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE, L"Courier"};

  DebugPrint("fillDEVINFO - enter\n");

  memset(pdi, 0, sizeof(DEVINFO));

  /*FLONG*/     pdi->flGraphicsCaps = (GCAPS_WINDINGFILL | GCAPS_GEOMETRICWIDE | GCAPS_OPAQUERECT | GCAPS_MONO_DITHER);
  /*LOGFONTW*/  pdi->lfDefaultFont = defaultFont;
  /*LOGFONTW*/  pdi->lfAnsiVarFont = ansiVarFont;
  /*LOGFONTW*/  pdi->lfAnsiFixFont = ansiFixFont;
  /*ULONG*/     pdi->cFonts = 0;
  /*ULONG*/     pdi->iDitherFormat = BMF_32BPP;
  /*USHORT*/    pdi->cxDither = 0;
  /*USHORT*/    pdi->cyDither = 0;
  /*HPALETTE*/  pdi->hpalDefault = EngCreatePalette(PAL_BITFIELDS, 0, 0, 0xFF0000, 0x00FF00, 0x0000FF);
      
#if (NTDDI_VERSION >= NTDDI_VISTA)
  /*FLONG*/     pdi->flGraphicsCaps2 = (GCAPS2_INCLUDEAPIBITMAPS | GCAPS2_EXCLUDELAYERED);
#endif

  DebugPrint("fillDEVINFO - exit\n");

  return TRUE;
}
