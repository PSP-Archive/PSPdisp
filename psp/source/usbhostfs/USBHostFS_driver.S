	.set noreorder

#include "pspstub.s"

	STUB_START "USBHostFS_driver",0x00090000,0x000A0005
	STUB_FUNC  0x75246D41,usbAsyncRegister
	STUB_FUNC  0x587DDEDA,usbAsyncUnregister
	STUB_FUNC  0xBA9F1145,usbAsyncRead
	STUB_FUNC  0xE4C00162,usbAsyncReadWithTimeout
	STUB_FUNC  0x5D1F19A0,usbAsyncWrite
	STUB_FUNC  0x89D91015,usbAsyncFlush
	STUB_FUNC  0xFF18E862,usbWaitForConnect
	STUB_FUNC  0x4ABA9C2B,usbWriteBulkData
	STUB_FUNC  0x642EF71E,usbLockBus
	STUB_FUNC  0x439E6C6C,usbUnlockBus
	STUB_END
