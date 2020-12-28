/*
  ***************************************************
  PSPdisp (c) 2008 Jochen Schleu

  pspdisp.c - miniport driver

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"



BOOLEAN pspResetHW(PVOID HwDeviceExtension, ULONG Columns, ULONG Rows)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspResetHW - enter and exit\n"));
  return TRUE;
}

BOOLEAN pspVidInterrupt(PVOID HwDeviceExtension)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspVidInterrupt - enter and exit\n"));
  return TRUE;
}

VP_STATUS pspGetPowerState(PVOID HwDeviceExtension, ULONG HwId, PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspGetPowerState - enter and exit\n"));
  return NO_ERROR;
}

VP_STATUS pspSetPowerState(PVOID HwDeviceExtension, ULONG HwId, PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspSetPowerState - enter and exit\n"));
  return NO_ERROR;
}

ULONG pspGetChildDescriptor (IN PVOID HwDeviceExtension,IN PVIDEO_CHILD_ENUM_INFO ChildEnumInfo, OUT PVIDEO_CHILD_TYPE pChildType, OUT PVOID pChildDescriptor, OUT PULONG pUId, OUT PULONG pUnused)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspGetChildDescriptor - enter and exit\n"));
  return ERROR_NO_MORE_DEVICES;
}

VP_STATUS __checkReturn pspFindAdapter(__in PVOID HwDeviceExtension, __in PVOID HwContext, __in PWSTR ArgumentString, __inout_bcount(sizeof(VIDEO_PORT_CONFIG_INFO)) PVIDEO_PORT_CONFIG_INFO ConfigInfo, __out PUCHAR Again)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspFindAdapter - enter and exit\n"));
  return NO_ERROR;
}

BOOLEAN pspInitialize(PVOID HwDeviceExtension)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspInitialize - enter and exit\n"));
  return TRUE;
}

BOOLEAN pspStartIO(PVOID HwDeviceExtension, PVIDEO_REQUEST_PACKET RequestPacket)
{
  VideoDebugPrint((0, "PSPdisp miniport: pspStartIO - enter and exit\n"));
  return TRUE;
}


ULONG DriverEntry(PVOID Context1, PVOID Context2)
{
  VIDEO_HW_INITIALIZATION_DATA hwInitData;
  ULONG initializationStatus;

  VideoDebugPrint((0, "PSPdisp miniport: DriverEntry - enter\n"));

  // Zero out structure.

  VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

  // Specify sizes of structure and extension.

  hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

  // Set entry points.

  hwInitData.HwFindAdapter             = &pspFindAdapter;
  hwInitData.HwInitialize              = &pspInitialize;
  hwInitData.HwStartIO                 = &pspStartIO;
  hwInitData.HwResetHw                 = &pspResetHW;
  hwInitData.HwInterrupt               = &pspVidInterrupt;
  hwInitData.HwGetPowerState           = &pspGetPowerState;
  hwInitData.HwSetPowerState           = &pspSetPowerState;
  hwInitData.HwGetVideoChildDescriptor = &pspGetChildDescriptor;

  hwInitData.HwLegacyResourceList      = NULL; 
  hwInitData.HwLegacyResourceCount     = 0; 

  // No device extension necessary.
  hwInitData.HwDeviceExtensionSize = 0;
  hwInitData.AdapterInterfaceType = 0;

  initializationStatus = VideoPortInitialize(Context1, Context2, &hwInitData, NULL);

  if (initializationStatus != NO_ERROR) 
  {
    // Maybe we are running on W2k, lets try again with 
	// the appropriate HwInitDataSize value for W2k
    hwInitData.HwInitDataSize = SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA;
    initializationStatus = VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
  }

  VideoDebugPrint((0, "PSPdisp miniport: DriverEntry - exit with %d\n", initializationStatus));

  return initializationStatus;
}

