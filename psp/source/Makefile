TARGET = pspdisp

GLYPHS = glyphs/battery0.o glyphs/battery1.o glyphs/battery2.o glyphs/battery3.o glyphs/shoulder.o glyphs/leftright.o glyphs/updown.o glyphs/square.o glyphs/triangle.o glyphs/cross.o glyphs/circle.o
OBJS_EXT =  library/intraFont.o library/libccc.o library/danzeff.o
OBJS_PRX = kernel/kernel.o sio/sio.o usbhostfs/USBHostFS.o 
OBJS_INT = main.o menu.o audio.o com.o power.o config.o osk.o debug.o utils.o usb.o wlan.o graphic.o compress.o msgdlg.o
OBJS = $(GLYPHS) $(OBJS_EXT) $(OBJS_PRX) $(OBJS_INT)

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBS = -lpspaudio -lpng -lz -lpspusb -lpspgum -lpspgu -lm -lpsppower -lpspjpeg -lpsputility -lpspkubridge -lpsprtc -ljpeg

LIBDIR += libjpeg/
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSPdisp v0.6.1
PSP_EBOOT_ICON = ICON0.PNG

PSP_FW_VERSION = 371
BUILD_PRX = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

glyphs/battery0.o : glyphs/battery0.raw
	bin2o -i glyphs/battery0.raw glyphs/battery0.o glyphBattery0
	
glyphs/battery1.o : glyphs/battery1.raw
	bin2o -i glyphs/battery1.raw glyphs/battery1.o glyphBattery1
	
glyphs/battery2.o : glyphs/battery2.raw
	bin2o -i glyphs/battery2.raw glyphs/battery2.o glyphBattery2

glyphs/battery3.o : glyphs/battery3.raw
	bin2o -i glyphs/battery3.raw glyphs/battery3.o glyphBattery3

glyphs/cross.o : glyphs/cross.raw
	bin2o -i glyphs/cross.raw glyphs/cross.o glyphCross
	
glyphs/circle.o : glyphs/circle.raw
	bin2o -i glyphs/circle.raw glyphs/circle.o glyphCircle
	
glyphs/triangle.o : glyphs/triangle.raw
	bin2o -i glyphs/triangle.raw glyphs/triangle.o glyphTriangle
		
glyphs/square.o : glyphs/square.raw
	bin2o -i glyphs/square.raw glyphs/square.o glyphSquare
	
glyphs/updown.o : glyphs/updown.raw
	bin2o -i glyphs/updown.raw glyphs/updown.o glyphUpDown
	
glyphs/leftright.o : glyphs/leftright.raw
	bin2o -i glyphs/leftright.raw glyphs/leftright.o glyphLeftRight

glyphs/shoulder.o : glyphs/shoulder.raw
	bin2o -i glyphs/shoulder.raw glyphs/shoulder.o glyphShoulder