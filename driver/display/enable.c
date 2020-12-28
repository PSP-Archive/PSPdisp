/*
  ***************************************************
  PSPdisp (c) 2008 Jochen Schleu

  enable.c - display initialization code

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/


// This driver is heavily based on the "framebuffer" example
// from the Microsoft Windows 2003 DDK.

#include "debug.h"
#include "pspdisp.h"


// Functions implemented by the driver, only the bare minimum here.
static DRVFN gadrvfn[] =
{
  { INDEX_DrvDisableDriver,     (PFN) DrvDisableDriver     },
  { INDEX_DrvEnablePDEV,        (PFN) DrvEnablePDEV        },
  { INDEX_DrvCompletePDEV,      (PFN) DrvCompletePDEV      },
  { INDEX_DrvDisablePDEV,       (PFN) DrvDisablePDEV       },
  { INDEX_DrvEnableSurface,     (PFN) DrvEnableSurface     },
  { INDEX_DrvDisableSurface,    (PFN) DrvDisableSurface    },
  { INDEX_DrvGetModes,          (PFN) DrvGetModes          },
  { INDEX_DrvAssertMode,        (PFN) DrvAssertMode        },
  { INDEX_DrvEscape,            (PFN) DrvEscape            }, 
  { INDEX_DrvAlphaBlend,        (PFN) DrvAlphaBlend        }, 
  { INDEX_DrvBitBlt,            (PFN) DrvBitBlt            }, 
  { INDEX_DrvStretchBlt,        (PFN) DrvStretchBlt        }, 
  { INDEX_DrvTransparentBlt,    (PFN) DrvTransparentBlt    },
  { INDEX_DrvCopyBits,          (PFN) DrvCopyBits          },        
  { INDEX_DrvMovePointer,       (PFN) DrvMovePointer       },
  { INDEX_DrvSetPointerShape,   (PFN) DrvSetPointerShape   },
  { INDEX_DrvFillPath,          (PFN) DrvFillPath          },
  { INDEX_DrvStrokeAndFillPath, (PFN) DrvStrokeAndFillPath },
  { INDEX_DrvGradientFill,      (PFN) DrvGradientFill      },
  { INDEX_DrvLineTo,            (PFN) DrvLineTo            },
  { INDEX_DrvTextOut,           (PFN) DrvTextOut           }
};                                                   

const FLONG hooks = (HOOK_COPYBITS |
                     HOOK_ALPHABLEND |
                     HOOK_BITBLT | 
                     HOOK_FILLPATH |
                     HOOK_GRADIENTFILL |
                     HOOK_LINETO |
                     HOOK_STRETCHBLT | 
                     HOOK_STROKEANDFILLPATH |
                     HOOK_TEXTOUT |
                     HOOK_TRANSPARENTBLT
                     );
                    

// This function does everything the SDK samples do.
// We cannot just fill in the structure but have to check for the value of cj
// because the system can give a too small structure.
BOOL DrvEnableDriver(IN ULONG iEngineVersion, IN ULONG cj, OUT DRVENABLEDATA *pded)
{
  DebugPrint("DrvEnableDriver - enter with cj='%d'\n", cj);

  // Set the screen dirty variable to true at the start.
  screenIsDirty = 1;

  // Set the function table.
  if (cj >= sizeof(DRVENABLEDATA)) {
    pded->pdrvfn = gadrvfn;
  }

  // Set the number of entries in the function table.
  if (cj >= (sizeof(ULONG) * 2)) {
    pded->c = sizeof(gadrvfn) / sizeof(DRVFN);
  }

  // The driver should work on all NT version.
  if (cj >= sizeof(ULONG)) {
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;
  }

  DebugPrint("DrvEnableDriver - exit\n");
  return(TRUE);
}



// Free resources allocated in DrvEnableDriver, do nothing here.
VOID DrvDisableDriver(VOID)
{
  DebugPrint("DrvDisableDriver - enter\n");
  DebugPrint("DrvDisableDriver - exit\n");
  return;
}



// Create a private device structure based on the requested mode.
DHPDEV
  DrvEnablePDEV(
    IN DEVMODEW  *pdm,            // pointer to DEVMODE struct, contains driver data (?)
    IN LPWSTR    pwszLogAddress,  // ignore
    IN ULONG     cPat,            // ignore
    OUT HSURF    *phsurfPatterns, // ignore
    IN ULONG     cjCaps,          // size of the pdevcaps buffer (zero-filled)
    OUT ULONG    *pdevcaps,       // GDIINFO structure (why is this not of type GDIINFO?)
    IN ULONG     cjDevInfo,       // size of the pdi buffer
    OUT DEVINFO  *pdi,            // pointer to DEVINFO structure (zero-filled)
    IN HDEV      hdev,            // GDI supplied handle to the device, has to be stored
    IN LPWSTR    pwszDeviceName,  // name of the device (?)
    IN HANDLE    hDriver          // handle to the miniport for IOCTL
    )
{

  PPDEV ppdev = (PPDEV) NULL;

  DebugPrint("DrvEnablePDEV - enter\n");

  // Allocate a physical device structure, save all information about
  // the device here. This information is not used by Windows, only
  // the pointer to it must be known to the system.
  ppdev = (PPDEV) EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
  if (ppdev == (PPDEV) NULL) {
    DebugPrint("DrvEnablePDEV - exit with failed EngAllocMem for ppdev\n");
    return((DHPDEV) 0);
  }

  DebugPrint("DrvEnablePDEV - GDIINFO buffer size: %d, compiled size: %d\n", cjCaps, sizeof(GDIINFO));
  DebugPrint("DrvEnablePDEV - DEVINFO buffer size: %d, compiled size: %d\n", cjDevInfo, sizeof(DEVINFO));

  // Save the screen handle in the PDEV.
  ppdev->hDriver = hDriver;

  // Fill in the GDIINFO structure.
  if (fillGDIINFO((GDIINFO*)pdevcaps, pdm, ppdev)) {
    if (fillDEVINFO(pdi, pdm)) {
      // all done, return handle to device structure
      DebugPrint("DrvEnablePDEV - exit with success\n");
      return (DHPDEV)ppdev;
    }
  }
  
  DebugPrint("DrvEnablePDEV - exit with error\n");

  // some error occured, return NULL
  return((DHPDEV) 0);
}


// Store the hdev handle in the private device structure.
VOID DrvCompletePDEV(DHPDEV dhpdev, HDEV hdev)
{
  DebugPrint("DrvCompletePDEV - enter\n");

  ((PPDEV)dhpdev)->hdevEng = hdev;

  DebugPrint("DrvCompletePDEV - exit\n");
}


// Free the data allocated in DrvEnablePDEV.
VOID DrvDisablePDEV(DHPDEV dhpdev)
{
  DebugPrint("DrvDisablePDEV - enter\n");

  EngFreeMem((PPDEV)dhpdev);

  DebugPrint("DrvDisablePDEV - exit\n");
}


// Enable the device surface and hook function calls.
HSURF DrvEnableSurface(DHPDEV dhpdev)
{ 
  HBITMAP returnBitmap;
  SIZEL sizl;
  HSURF returnSurface;
  BOOL result;

  DebugPrint("DrvEnableSurface - enter\n");

  // set width and height to highest resolution
  sizl.cx = ((PPDEV)dhpdev)->width;
  sizl.cy = ((PPDEV)dhpdev)->height;

  DebugPrint("DrvEnableSurface - width = %d, height = %d\n", sizl.cx, sizl.cy);

  returnBitmap = EngCreateBitmap(sizl, sizl.cx * 4, BMF_32BPP, 0, 0);

  DebugPrint("DrvEnableSurface - EngCreateBitmap returned %d\n", returnBitmap);
  
  returnSurface = (HSURF)returnBitmap;

  // associate the surface to the device, don't hook any operations
  result = EngAssociateSurface(returnSurface, ((PPDEV)dhpdev)->hdevEng, hooks);

  DebugPrint("DrvEnableSurface - EngAssociateSurface returned %d\n", result);

  ((PPDEV)dhpdev)->hsurfEng = returnSurface;

  bitmapSurface = returnSurface;

  DebugPrint("DrvEnableSurface - exit\n");

  return returnSurface;
}



// Delete the GDI bitmap.
VOID DrvDisableSurface(IN DHPDEV dhpdev)
{
  DebugPrint("DrvDisableSurface - enter\n");

  EngDeleteSurface(((PDEV*)dhpdev)->hsurfEng);

  DebugPrint("DrvDisableSurface - exit\n");
}



// Returns the available display modes.
ULONG DrvGetModes(IN HANDLE hDriver, IN ULONG cjSize, OUT DEVMODEW *pdm)
{
  int numberOfModes = MODE_COUNT * 2;

  // There are 5 supported modes
  ULONG neededSize = sizeof(DEVMODEW) * numberOfModes;
  
  int i;

  DebugPrint("DrvGetModes - enter with cjSize='%d' OK\n", cjSize);

  // This function is first called with (pdm == 0), then
  // we have to return the function with as much memory as we
  // need to store all modes.
  if (pdm == NULL) 
  {
    // Return the size of 1*DEVMODEW.
    DebugPrint("DrvGetModes - exit with %d size request\n", neededSize);
    return neededSize;
  }

  for (i = 0; i < numberOfModes; i++) 
  {
    memset(pdm, 0, sizeof(DEVMODEW));

    memcpy(pdm->dmDeviceName, L"pspdisp", sizeof(L"pspdisp"));

    pdm->dmSpecVersion      = DM_SPECVERSION;
    pdm->dmDriverVersion    = DM_SPECVERSION;
    pdm->dmSize             = sizeof(DEVMODEW);
    pdm->dmDriverExtra      = 0;

    pdm->dmBitsPerPel       = 32;

    if (i >= MODE_COUNT) 
    {
        pdm->dmPelsWidth        = displayModes[i - MODE_COUNT].height;
        pdm->dmPelsHeight       = displayModes[i - MODE_COUNT].width;
    }
    else
    {
        pdm->dmPelsWidth        = displayModes[i].width;
        pdm->dmPelsHeight       = displayModes[i].height;
    }

	  pdm->dmDisplayFrequency = 60;
	  pdm->dmDisplayFlags     = 0;
	  pdm->dmPanningWidth     = pdm->dmPelsWidth;
	  pdm->dmPanningHeight    = pdm->dmPelsHeight;
	  pdm->dmFields           = DM_BITSPERPEL       |
                              DM_PELSWIDTH        |
                              DM_PELSHEIGHT       |
                              DM_DISPLAYFREQUENCY |
                              DM_DISPLAYFLAGS;
	  pdm++;
  }

  DebugPrint("DrvGetModes - exit with settings written\n");

  return neededSize;
}




// This isn't usually called, but shouldn't fail anyway.
BOOL DrvAssertMode(IN DHPDEV dhpdev, IN BOOL bEnable)
{
  DebugPrint("DrvAssertMode - enter and exit\n");

  return TRUE;
}



// Process messages sent from user mode
ULONG DrvEscape(IN SURFOBJ *pso, IN ULONG iEsc, IN ULONG cjIn, IN PVOID pvIn, IN ULONG cjOut, OUT PVOID pvOut)
{
  DebugPrint("DrvEscape pso=%u, iEsc=%u, cjIn=%u, pvIn=%u, cjOut=%u, pvOut=%u\n", pso, iEsc, cjIn, pvIn, cjOut, pvOut);

  if (iEsc == ESCAPE_IS_DIRTY)
  {
    // Requested to signal if the screen has changed since the last DrvEscape call
    // Check other parameter sizes
    if ((cjIn == 0) && (cjOut == 4))
    {
      (*(unsigned int*)pvOut) = screenIsDirty;
      screenIsDirty = 0;
      
      return 1;
    }
  }

  return 0;
}






// Write updated counter into the memory mapped file (if enabled)
void WriteUpdateNotification()
{
  screenIsDirty = 1;
}



// Hooking all drawing functions to get nofitication of display updates.
// Interesting thing to note is that hooking DrvCopyBits() alone
// already seems to catch 99% of the screen changes.


BOOL
  DrvAlphaBlend(
    IN SURFOBJ  *psoDest,
    IN SURFOBJ  *psoSrc,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN RECTL  *prclDest,
    IN RECTL  *prclSrc,
    IN BLENDOBJ  *pBlendObj
    )
{
  WriteUpdateNotification();
  return EngAlphaBlend(psoDest, psoSrc, pco, pxlo, prclDest, prclSrc, pBlendObj);
}


BOOL
  DrvBitBlt(
    IN SURFOBJ  *psoTrg,
    IN SURFOBJ  *psoSrc,
    IN SURFOBJ  *psoMask,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN RECTL  *prclTrg,
    IN POINTL  *pptlSrc,
    IN POINTL  *pptlMask,
    IN BRUSHOBJ  *pbo,
    IN POINTL  *pptlBrush,
    IN ROP4  rop4
    )
{
  WriteUpdateNotification();
  return EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);
}


BOOL
  DrvStretchBlt(
    IN SURFOBJ  *psoDest,
    IN SURFOBJ  *psoSrc,
    IN SURFOBJ  *psoMask,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN COLORADJUSTMENT  *pca,
    IN POINTL  *pptlHTOrg,
    IN RECTL  *prclDest,
    IN RECTL  *prclSrc,
    IN POINTL  *pptlMask,
    IN ULONG  iMode
    )
{
  WriteUpdateNotification();
  return EngStretchBlt(psoDest, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDest, prclSrc, pptlMask, iMode);
}


BOOL
  DrvTransparentBlt(
    IN SURFOBJ  *psoDest,
    IN SURFOBJ  *psoSrc,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN RECTL  *prclDst,
    IN RECTL  *prclSrc,
    IN ULONG  iTransColor,
    IN ULONG  ulReserved
    )
{
  WriteUpdateNotification();
  return EngTransparentBlt(psoDest, psoSrc, pco, pxlo, prclDst, prclSrc, iTransColor, ulReserved);
}


BOOL
  DrvCopyBits(
    IN SURFOBJ  *psoDest,
    IN SURFOBJ  *psoSrc,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN RECTL  *prclDest,
    IN POINTL  *pptlSrc
    )
{
  if (psoSrc != NULL) {
    if (psoSrc->hsurf != bitmapSurface) {
      WriteUpdateNotification();
    }
  }

  return EngCopyBits(psoDest, psoSrc, pco, pxlo, prclDest, pptlSrc);
}


VOID
  DrvMovePointer(
    IN SURFOBJ  *pso,
    IN LONG  x,
    IN LONG  y,
    IN RECTL  *prcl
    )
{
  WriteUpdateNotification();
  EngMovePointer(pso, x, y, prcl);
}


ULONG
  DrvSetPointerShape(
    IN SURFOBJ  *pso,
    IN SURFOBJ  *psoMask,
    IN SURFOBJ  *psoColor,
    IN XLATEOBJ  *pxlo,
    IN LONG  xHot,
    IN LONG  yHot,
    IN LONG  x,
    IN LONG  y,
    IN RECTL  *prcl,
    IN FLONG  fl
    )
{
  WriteUpdateNotification();
  return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
}


BOOL
  DrvFillPath(
    IN SURFOBJ  *pso,
    IN PATHOBJ  *ppo,
    IN CLIPOBJ  *pco,
    IN BRUSHOBJ  *pbo,
    IN POINTL  *pptlBrushOrg,
    IN MIX  mix,
    IN FLONG  flOptions
    )
{
  WriteUpdateNotification();
  return EngFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);
}


BOOL
  DrvStrokeAndFillPath(
    IN SURFOBJ  *pso,
    IN PATHOBJ  *ppo,
    IN CLIPOBJ  *pco,
    IN XFORMOBJ  *pxo,
    IN BRUSHOBJ  *pboStroke,
    IN LINEATTRS  *plineattrs,
    IN BRUSHOBJ  *pboFill,
    IN POINTL  *pptlBrushOrg,
    IN MIX  mixFill,
    IN FLONG  flOptions
    )
{
  WriteUpdateNotification();
  return EngStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, plineattrs, pboFill, pptlBrushOrg, mixFill, flOptions);
}


BOOL
  DrvGradientFill(
    IN SURFOBJ  *psoDest,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN TRIVERTEX  *pVertex,
    IN ULONG  nVertex,
    IN PVOID  pMesh,
    IN ULONG  nMesh,
    IN RECTL  *prclExtents,
    IN POINTL  *pptlDitherOrg,
    IN ULONG  ulMode
    )
{
  WriteUpdateNotification();
  return EngGradientFill(psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);
}


BOOL
  DrvLineTo(
    SURFOBJ  *pso,
    CLIPOBJ  *pco,
    BRUSHOBJ  *pbo,
    LONG  x1,
    LONG  y1,
    LONG  x2,
    LONG  y2,
    RECTL  *prclBounds,
    MIX  mix
    )
{
  WriteUpdateNotification();
  return EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
}


BOOL
  DrvTextOut(
    IN SURFOBJ  *pso,
    IN STROBJ  *pstro,
    IN FONTOBJ  *pfo,
    IN CLIPOBJ  *pco,
    IN RECTL  *prclExtra,
    IN RECTL  *prclOpaque,
    IN BRUSHOBJ  *pboFore,
    IN BRUSHOBJ  *pboOpaque,
    IN POINTL  *pptlOrg,
    IN MIX  mix
    )
{
  WriteUpdateNotification();
  return EngTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);
}











