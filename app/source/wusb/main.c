/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  main.c - WinUsb and SetupAPI wrapper

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include <stdio.h>
#include <windows.h>
#include <winusb.h>
#include <setupapi.h>

#define DllExport __declspec( dllexport )

HMODULE libraryHandle = NULL;


typedef BOOL (__stdcall *T_WinUsb_Initialize)(HANDLE, PWINUSB_INTERFACE_HANDLE);
typedef BOOL (__stdcall *T_WinUsb_Free)(WINUSB_INTERFACE_HANDLE);
typedef BOOL (__stdcall *T_WinUsb_SetPipePolicy)(WINUSB_INTERFACE_HANDLE, UCHAR, ULONG, ULONG, PVOID);
typedef BOOL (__stdcall *T_WinUsb_ReadPipe)(WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG, LPOVERLAPPED);
typedef BOOL (__stdcall *T_WinUsb_WritePipe)(WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG, LPOVERLAPPED);

T_WinUsb_Initialize P_WinUsb_Initialize = NULL;
T_WinUsb_Free P_WinUsb_Free = NULL;
T_WinUsb_SetPipePolicy P_WinUsb_SetPipePolicy = NULL;
T_WinUsb_ReadPipe P_WinUsb_ReadPipe = NULL;
T_WinUsb_WritePipe P_WinUsb_WritePipe = NULL;




DllExport BOOL WUsb_Initialize(IN HANDLE DeviceHandle, OUT PWINUSB_INTERFACE_HANDLE InterfaceHandle)
{
  if (P_WinUsb_Initialize)
    return (*P_WinUsb_Initialize)(DeviceHandle, InterfaceHandle);
  else
    return FALSE;
}


DllExport BOOL WUsb_Free(IN WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
  if (P_WinUsb_Free)
    return (*P_WinUsb_Free)(InterfaceHandle);
  else
    return FALSE;
}


DllExport BOOL WUsb_SetPipePolicy(IN WINUSB_INTERFACE_HANDLE InterfaceHandle, IN UCHAR PipeID, IN ULONG PolicyType, IN ULONG ValueLength, IN PVOID Value)
{
  if (P_WinUsb_SetPipePolicy)
    return (*P_WinUsb_SetPipePolicy)(InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
  else
    return FALSE;
}


DllExport BOOL WUsb_ReadPipe(IN WINUSB_INTERFACE_HANDLE InterfaceHandle, IN UCHAR PipeID, IN PUCHAR Buffer, IN ULONG BufferLength, OUT PULONG LengthTransferred, IN LPOVERLAPPED Overlapped)
{
  if (P_WinUsb_ReadPipe)
    return (*P_WinUsb_ReadPipe)(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
  else
    return FALSE;
}


DllExport BOOL WUsb_WritePipe(IN WINUSB_INTERFACE_HANDLE InterfaceHandle, IN UCHAR PipeID, IN PUCHAR Buffer, IN ULONG BufferLength, OUT PULONG LengthTransferred, IN LPOVERLAPPED Overlapped)
{
  if (P_WinUsb_WritePipe)
    return (*P_WinUsb_WritePipe)(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
  else
    return FALSE;
}




DllExport BOOL WUsb_GetDevicePath(IN char* guidString, OUT char* devicePath)
{
  GUID guid;
  SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
  HDEVINFO devInfo;
  DWORD requiredSize;
  SP_DEVINFO_DATA devInfoData;
  PSP_INTERFACE_DEVICE_DETAIL_DATA deviceDetail;
  
  unsigned long guidBytes[8];
  int i;

  // Extract GUID
  if (11 != sscanf(guidString, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", &guid.Data1, &guid.Data2, &guid.Data3, &guidBytes[0], &guidBytes[1], &guidBytes[2], &guidBytes[3], &guidBytes[4], &guidBytes[5], &guidBytes[6], &guidBytes[7]))
    return FALSE;

  for (i = 0; i < 8; i++)
    guid.Data4[i] = (unsigned char)guidBytes[i];

  devInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (devInfo == INVALID_HANDLE_VALUE) 
    return FALSE;

  // Get first present device
  deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

  if (TRUE != SetupDiEnumDeviceInterfaces(devInfo, NULL, &guid, 0, &deviceInterfaceData))
  {
    SetupDiDestroyDeviceInfoList(devInfo);
    return FALSE;
  }

  // Get buffer size for interface data
  if (TRUE != SetupDiGetDeviceInterfaceDetail(devInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL))
  {
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return FALSE;
  }

  // Get interface data
  deviceDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredSize);
  
  deviceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
  devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
  
  if (TRUE != SetupDiGetDeviceInterfaceDetail(devInfo, &deviceInterfaceData, deviceDetail, requiredSize, NULL, &devInfoData))
  {
    free(deviceDetail);
    return FALSE;
  }

  strcpy(devicePath, deviceDetail->DevicePath);
  
  free(deviceDetail);
  
  return TRUE;
}



BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
  switch( fdwReason ) 
  { 
    case DLL_PROCESS_ATTACH:
      libraryHandle = LoadLibrary("winusb.dll");
      if (libraryHandle)
      {
        P_WinUsb_Initialize = (T_WinUsb_Initialize)GetProcAddress(libraryHandle, "WinUsb_Initialize");
        P_WinUsb_Free = (T_WinUsb_Free)GetProcAddress(libraryHandle, "WinUsb_Free");
        P_WinUsb_SetPipePolicy = (T_WinUsb_SetPipePolicy)GetProcAddress(libraryHandle, "WinUsb_SetPipePolicy");
        P_WinUsb_ReadPipe = (T_WinUsb_ReadPipe)GetProcAddress(libraryHandle, "WinUsb_ReadPipe");
        P_WinUsb_WritePipe = (T_WinUsb_WritePipe)GetProcAddress(libraryHandle, "WinUsb_WritePipe");
      }
      break;

    case DLL_PROCESS_DETACH:
      if (libraryHandle)
        FreeLibrary(libraryHandle);
      break;
  }
  return TRUE;
}
