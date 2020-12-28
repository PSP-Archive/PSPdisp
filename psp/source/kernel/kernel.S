	.set noreorder

#include "pspstub.s"

	STUB_START "kernel",0x40090000,0x00050005
	STUB_FUNC  0x8CB4795E,kernelSetWlanLedState
	STUB_FUNC  0x46EAFFAA,kernelDisplaySetBrightness
	STUB_FUNC  0xF9B7254E,kernelDisplayGetBrightness
	STUB_FUNC  0xD49E896E,kernelCtrlReadBufferPositive
	STUB_FUNC  0x8589F358,kernelGetModel
	STUB_END
