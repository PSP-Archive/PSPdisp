	.set noreorder

#include "pspstub.s"

	STUB_START "sio",0x40090000,0x00030005
	STUB_FUNC  0xFA93DDC7,kernelDebugSioInit
	STUB_FUNC  0x104ECB9E,kernelDebugSioTerm
	STUB_FUNC  0x0A1CB655,kernelDebugSioPutText
	STUB_END
