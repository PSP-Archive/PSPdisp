//-----------------------------------------------------------------------
// <copyright file="DeviceSpecific.cpp" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//
// Module:
//      DeviceSpecific.cpp
//
// Description:
//      Implements the device specific code for the driver.
//
//      This file contains device specific information, for example, how
//      to send data to the XYZ device as opposed to the ABC device.
//
//      This sample implementation doesn't do anything - it only contains
//      the stubs to be implemented later
//
//-----------------------------------------------------------------------

#include "Common.h"
#include <stdio.h>
#include <sddl.h>
#include <windows.h>
#include <shellapi.h>
#include <propvarutil.h>


DWORD WINAPI ControlThread( LPVOID lpParam );

extern bool g_fDeviceIsValid;

#define SIDESHOW_BUTTON_PREVIOUS 0
#define SIDESHOW_BUTTON_NEXT 1
#define SIDESHOW_BUTTON_UP 2
#define SIDESHOW_BUTTON_DOWN 3
#define SIDESHOW_BUTTON_SELECT 4
#define SIDESHOW_BUTTON_MENU 5
#define SIDESHOW_BUTTON_BACK 6
#define SIDESHOW_FORCE_REFRESH 100

#define SIDESHOW_MSG_QUIT 0xFFFFFFFF


typedef struct {
    int counter;
    int button;
} controlInput;

HANDLE controlThreadHandle;

int currentOutputCount;
bool quitSideShowMode;

HANDLE outputMemoryFileHandle;
LPVOID outputMemoryFileStartAddress;
const LPCTSTR outputMemoryFile = _T("Global\\a217007d-6605-4289-bbc6-b4d32ff10bd7");
const int outputMemoryFileSize = 391734 + 4; // 480 * 272 * 3 + DIB header + int change counter

HANDLE inputMemoryFileHandle;
LPVOID inputMemoryFileStartAddress;
const LPCTSTR inputMemoryFile = _T("Global\\45f7a9bc-711e-4012-9d8f-d4d89ec8b00b");
const int inputMemoryFileSize = 8; // 2 ints


UINT32 GetDeviceParamUInt32(IWDFNamedPropertyStore *pPropStore, LPCWSTR pwszParamName, UINT32 nDefaultValue = 0);
LPWSTR GetDeviceParamString(IWDFNamedPropertyStore *pPropStore, LPCWSTR pwszParamName, LPCWSTR pwszDefault = NULL);
HRESULT SetDeviceParamDWORD(LPCWSTR pwszParamName, DWORD dwValue = 0);

static IWDFNamedPropertyStore* s_pPropStore = NULL;



HRESULT DeviceSpecificDisplayBitmap(const BYTE* pbBitmapData, DWORD cbBitmapData, DWORD dwRenderFlags)
{
    // Send a bitmap to your device here.
    // The bitmap is in buffer pbBitmapData of size (in bytes) cbBitmapData
    // For Example: SendBitmap(pbBitmapData, cbBitmapData);

    currentOutputCount++;

    if (currentOutputCount > 0x00FFFFF)
      currentOutputCount = 0;

    unsigned int outputCount;
    unsigned int currentlyOutOfGlanceMenu = (g_pDataManager->GetCurrentContentId() > 0) ? 1 : 0; 

    if (quitSideShowMode)
      outputCount = SIDESHOW_MSG_QUIT;
    else
      outputCount = currentOutputCount + (currentlyOutOfGlanceMenu << 24);

    memcpy(outputMemoryFileStartAddress, &outputCount, 4);
    memcpy((void*)((long long int)outputMemoryFileStartAddress + 4), pbBitmapData, cbBitmapData);

    quitSideShowMode = FALSE;

    return S_OK;
}



HRESULT RefreshCounter()
{
    currentOutputCount++;

    if (currentOutputCount > 0x00FFFFF)
      currentOutputCount = 0;

    unsigned int outputCount;
    unsigned int currentlyOutOfGlanceMenu = (g_pDataManager->GetCurrentContentId() > 0) ? 1 : 0; 

    if (quitSideShowMode)
      outputCount = SIDESHOW_MSG_QUIT;
    else
      outputCount = currentOutputCount + (currentlyOutOfGlanceMenu << 24);

    memcpy(outputMemoryFileStartAddress, &outputCount, 4);

    quitSideShowMode = FALSE;

    return S_OK;
}



HRESULT DeviceDisplayInitialization(PCWSTR pwszDeviceInstanceId, IWDFNamedPropertyStore *pPropStore)
{
    UNREFERENCED_PARAMETER(pwszDeviceInstanceId);

    if (NULL == s_pPropStore)
    {
        s_pPropStore = pPropStore;
        s_pPropStore->AddRef();
    }

    // Optionally check if the device has already been initialized
    // If it has, don't initialize a second time
    // Make sure to set this variable to true after initializing.
    if (true == g_fDeviceIsValid)
    {
        return E_FAIL;
    }

    // Initialize your device here.
    quitSideShowMode = FALSE;

    // Set the device capabilities
    CDevice::SetDeviceCaps(L"PSPdisp SideShow Device", // wszDeviceFriendlyName
                           L"JJS", // wszDeviceManufacturer
                           L"0.5.0", // wszDeviceFirmwareVersion
                           480, // dwDeviceHorizontalResolution
                           272, // dwDeviceVerticalResolution
                           24, // dwDeviceBitDepth
                           FALSE); // bTopDownBitmap

    // Set the renderer capabilities
    CRenderer::SetRendererCaps(L"Tahoma", // wszFontName
                               11, // wFontSizeInPixels
                               18, // wTitlebarHeight
                               13, // wMenuItemHeight
                               13, // wScrollAmountInPixels
                               GetDeviceParamUInt32(pPropStore, L"DefaultThemeIndex", 1)); // dwDefaultThemeIndex


    // Create security attributes to share memory between 
    // the service and an user mode application
    SECURITY_ATTRIBUTES secAttr;
    char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
    secAttr.nLength = sizeof(secAttr);
    secAttr.bInheritHandle = FALSE;
    secAttr.lpSecurityDescriptor = &secDesc;
    InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
    
    PSECURITY_DESCRIPTOR pSD;
    ConvertStringSecurityDescriptorToSecurityDescriptor(_T("S:(ML;;NW;;;LW)"), SDDL_REVISION_1, &pSD, NULL);
    PACL pSacl = NULL;
    BOOL fSaclPresent = FALSE;
    BOOL fSaclDefaulted = FALSE;
    GetSecurityDescriptorSacl(pSD, &fSaclPresent, &pSacl, &fSaclDefaulted);
    SetSecurityDescriptorSacl(secAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE);

    // create a memory mapped output file for sending the bitmap
    outputMemoryFileHandle = CreateFileMapping(
        INVALID_HANDLE_VALUE,           // system paging file
        &secAttr,                       // security attributes
        PAGE_READWRITE,                 // protection
        0,                              // high-order DWORD of size
        outputMemoryFileSize,           // low-order DWORD of size
        outputMemoryFile);              // name

    outputMemoryFileStartAddress = MapViewOfFile(
            outputMemoryFileHandle,     // handle to file-mapping object
            FILE_MAP_ALL_ACCESS,        // desired access
            0,
            0,
            0);                         // map all file


    if (!outputMemoryFileHandle)
    {
        OutputDebugString(L"CreateFileMapping for output failed!");
        return E_FAIL;
    }
    else
    {
        OutputDebugString(L"CreateFileMapping for output succeeded!");
        return S_OK;
    }
}


HRESULT DeviceButtonsInitialization(void)
{
    // Initialize button callbacks here.
    // This might be a thread, or registering a callback function.
    // Start Read Thread

    // Create security attributes to share memory between 
    // the service and an user mode application
    SECURITY_ATTRIBUTES secAttr;
    char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
    secAttr.nLength = sizeof(secAttr);
    secAttr.bInheritHandle = FALSE;
    secAttr.lpSecurityDescriptor = &secDesc;
    InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
    
    PSECURITY_DESCRIPTOR pSD;
    ConvertStringSecurityDescriptorToSecurityDescriptor(_T("S:(ML;;NW;;;LW)"), SDDL_REVISION_1, &pSD, NULL);
   
    PACL pSacl = NULL;
    BOOL fSaclPresent = FALSE;
    BOOL fSaclDefaulted = FALSE;
    GetSecurityDescriptorSacl(pSD, &fSaclPresent, &pSacl, &fSaclDefaulted);
    SetSecurityDescriptorSacl(secAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE);

    // create a memory mapped input file for receiving control input
    inputMemoryFileHandle = CreateFileMapping(
        INVALID_HANDLE_VALUE,           // system paging file
        &secAttr,                       // security attributes
        PAGE_READWRITE,                 // protection
        0,                              // high-order DWORD of size
        inputMemoryFileSize,            // low-order DWORD of size
        inputMemoryFile);               // name

    inputMemoryFileStartAddress = MapViewOfFile(
            inputMemoryFileHandle,     // handle to file-mapping object
            FILE_MAP_ALL_ACCESS,        // desired access
            0,
            0,
            0);                         // map all file

    if (!inputMemoryFileHandle)
    {
        OutputDebugString(L"CreateFileMapping for input failed!");
        return E_FAIL;
    }
    else
    {
        OutputDebugString(L"CreateFileMapping for input succeeded!");
    }

    ZeroMemory(inputMemoryFileStartAddress, 8);

    controlThreadHandle = CreateThread( NULL, 0, ControlThread, NULL, 0, NULL);  
    if ( controlThreadHandle == NULL)
    {
        OutputDebugString(_T("Create thread failed!"));
        return E_FAIL;
    }
    else
    {
        OutputDebugString(_T("Create thread succeeded!"));
        return S_OK;
    }
}


HRESULT DeviceDisplayShutdown(void)
{
    // Optionally check if the device has already been shutdown
    // If it has, don't shutdown a second time
    // Make sure to set this variable to false after shutting down.
    if (false == g_fDeviceIsValid)
    {
        return E_FAIL;
    }

    // Shutdown the device here.

    CRenderer::ReleaseRendererCaps();
    CDevice::ReleaseDeviceCaps();

    if (outputMemoryFileHandle)
    {
        if (outputMemoryFileStartAddress)
        {
            UnmapViewOfFile(outputMemoryFileStartAddress);
        }
        CloseHandle(outputMemoryFileHandle);
    }

    if (inputMemoryFileHandle)
    {
        if (inputMemoryFileStartAddress)
        {
            UnmapViewOfFile(inputMemoryFileStartAddress);
        }
        CloseHandle(inputMemoryFileHandle);
    }

    s_pPropStore->Release();
    s_pPropStore = NULL;

    return S_OK;
}


HRESULT DeviceButtonsShutdown(void)
{
    // Shutdown button callbacks here.
    // This might be stopping the thread or unregistering the callback

    if (inputMemoryFileHandle)
    {
        if (inputMemoryFileStartAddress)
        {
            UnmapViewOfFile(inputMemoryFileStartAddress);
        }
        CloseHandle(inputMemoryFileHandle);
    }

    return S_OK;
}



DWORD WINAPI ControlThread( LPVOID lpParam ) 
{
    int lastCounterValue = 0;

    while (1)
    {
        if (((controlInput*)inputMemoryFileStartAddress)->counter != lastCounterValue)
        {
            switch (((controlInput*)inputMemoryFileStartAddress)->button)
            {
                case SIDESHOW_FORCE_REFRESH:
                    RefreshCounter();
                    break;
                case SIDESHOW_BUTTON_PREVIOUS:
                    CDevice::HandleDeviceEvent(CDevice::ButtonPrevious);
                    break;
                case SIDESHOW_BUTTON_NEXT:
                    CDevice::HandleDeviceEvent(CDevice::ButtonNext);
                    break;
                case SIDESHOW_BUTTON_UP:
                    CDevice::HandleDeviceEvent(CDevice::ButtonUp);
                    break;
                case SIDESHOW_BUTTON_DOWN:
                    CDevice::HandleDeviceEvent(CDevice::ButtonDown);
                    break;
                case SIDESHOW_BUTTON_MENU:
                    CDevice::HandleDeviceEvent(CDevice::ButtonMenu);
                    break;
                case SIDESHOW_BUTTON_BACK:
                    CDevice::HandleDeviceEvent(CDevice::ButtonBack);
                    break;
                case SIDESHOW_BUTTON_SELECT:
                    CDevice::HandleDeviceEvent(CDevice::ButtonSelect);
                    break;
                default:
                    OutputDebugString(_T("unknown button code"));
                    break;
            }

            lastCounterValue = ((controlInput*)inputMemoryFileStartAddress)->counter;
        }

        Sleep(20);
    }

    return 0;
}



HRESULT ConfigureDevice()
{
    // The user picked the "Configure" option from the context menu of the home screen of the device

    // Do configuration work here. For example, open the SideShow Control Panel.
    
    //ShellExecuteA(GetDesktopWindow(), NULL, "control.exe", "/name Microsoft.WindowsSideShow", NULL, SW_SHOW);
  
    return S_OK;
}



HRESULT QuitSideShowMode()
{
    // Send signal for quitting SideShow mode and returning to desktop mirroring
    quitSideShowMode = TRUE;

    return S_OK;
}




// Helper functions

UINT32 GetDeviceParamUInt32(IWDFNamedPropertyStore *pPropStore, LPCWSTR pwszParamName, UINT32 nDefaultValue)
{
    PROPVARIANT propVar;
    HRESULT hr;
    UINT32 nRetVal = nDefaultValue;

    PropVariantInit(&propVar);

    hr = pPropStore->GetNamedValue(pwszParamName, &propVar);

    if (SUCCEEDED(hr))
    {
        nRetVal = PropVariantToUInt32WithDefault(propVar, nDefaultValue);

        PropVariantClear(&propVar);
    }

    return nRetVal;
}

HRESULT SetDeviceParamDWORD(LPCWSTR pwszParamName, DWORD dwValue)
{
    PROPVARIANT propVar;
    PropVariantInit(&propVar);
    propVar.vt = VT_UI4;
    propVar.ulVal = dwValue;

    HRESULT hr = s_pPropStore->SetNamedValue(pwszParamName, &propVar);

    PropVariantClear(&propVar);

    return hr;
}


// caller must CoTaskMemFree
LPWSTR GetDeviceParamString(IWDFNamedPropertyStore *pPropStore, LPCWSTR pwszParamName, LPCWSTR pwszDefault)
{
    PROPVARIANT propVar;
    HRESULT hr;
    LPWSTR pwszRetVal = NULL;

    PropVariantInit(&propVar);

    hr = pPropStore->GetNamedValue(pwszParamName, &propVar);

    if (SUCCEEDED(hr))
    {
        PropVariantToStringAlloc(propVar, &pwszRetVal);

        PropVariantClear(&propVar);
    }

    if (!pwszRetVal && pwszDefault)
    {
        size_t cch = wcslen(pwszDefault) + 1;
        pwszRetVal = (LPWSTR)CoTaskMemAlloc(cch * sizeof(WCHAR));

        if (pwszRetVal)
        {
            StringCchCopy(pwszRetVal, cch, pwszDefault);
        }
    }

    return pwszRetVal;
}
