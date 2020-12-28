//-----------------------------------------------------------------------
// <copyright file="DeviceSpecific.h" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//
// Module:
//      DeviceSpecific.h
//
// Description:
//      This header contains device specific information, for example,
//      how to send data to the XYZ device as opposed to the ABC device.
//
//-----------------------------------------------------------------------


HRESULT DeviceSpecificDisplayBitmap(const BYTE* pbBitmapData, DWORD cbBitmapData, DWORD dwRenderFlags);

HRESULT DeviceDisplayInitialization(PCWSTR pwszInstanceId, IWDFNamedPropertyStore *pPropStore);
HRESULT DeviceButtonsInitialization(void);
HRESULT DeviceDisplayShutdown(void);
HRESULT DeviceButtonsShutdown(void);
HRESULT ConfigureDevice();
HRESULT QuitSideShowMode();
