ver = debug
platform = mac32
VERSION = 1.3.33
FakeUSB = n
#20190306 1.0.12
#20221104 1.3.27 SDK v1.27
#20230906 1.3.30 SDK v1.30
#20231129 1.3.32 SDK v1.32
#20240103 1.3.32 SDK v1.33
CC = i686-pc-linux-gnu-g++
AR= ar

sub:=libusb
APPLE = 0

ifeq ($(platform), mac32)
APPLE = 1
endif

ifeq ($(platform), mac64)
APPLE = 1
endif

ifeq ($(platform), mac)
APPLE = 1
endif

#ifeq ($(APPLE), 1)
#a=$(shell otool -L ../../../linux/lib/$(platform)/libASICamera2.dylib)
#else
#a=$(shell ldd ../../../linux/lib/x86/libASICamera2.so)
#endif

a=$(shell nm -u ../../../linux/lib/$(platform)/libASICamera2.a |grep libusb_init)


$(warning $(sub)) 
$(warning $(a))
b:=$(findstring $(sub),$(a))
$(warning $(b))
#ifeq ($strip $(b)),)
ifeq ($(b),)
$(warning $(sub) is not substring of a)
FakeUSB = y
USB = -framework IOKit -framework foundation
else
$(warning $(sub) is substring of a)

USB = -lusb-1.0 -L../../../linux/libusb/$(platform)
endif

ifeq ($(customer), y)
sdkname = Veroptics
endif



ifeq ($(ver), debug)
DEFS = -D_LIN -D_DEBUG
CFLAGS = -fPIC -g $(DEFS) $(USB)
else
DEFS = -D_LIN
CFLAGS = -fPIC -O3 $(DEFS) $(USB)
endif

ifeq ($(platform), mac32)
CC = g++
CFLAGS += -D_MAC
CFLAGS += -m32
endif

ifeq ($(platform), mac64)
CC = g++
CFLAGS += -D_MAC
CFLAGS += -m64
endif

ifeq ($(platform), mac)
CC = g++
CFLAGS += -D_MAC
CFLAGS += -arch i386 -arch x86_64
endif

ifeq ($(platform), x86)
CFLAGS += -m32
CFLAGS += -msse 
CFLAGS += -mno-sse2
endif

ifeq ($(platform), x64)
CFLAGS += -m64
CFLAGS += -msse 
CFLAGS += -mno-sse2
endif

ifeq ($(platform), armv5)
CC = arm-none-linux-gnueabi-g++
AR= arm-none-linux-gnueabi-ar
CFLAGS += -march=armv5
endif

ifeq ($(platform), armv6)
CC = arm-bcm2708hardfp-linux-gnueabi-g++
AR= arm-bcm2708hardfp-linux-gnueabi-ar
CFLAGS += -march=armv6
endif
ifeq ($(platform), armv7)
CC = arm-linux-gnueabihf-g++
AR= arm-linux-gnueabihf-ar
CFLAGS += -march=armv7 -mcpu=cortex-m3 -mthumb
endif

ifeq ($(platform), armv8)
CC = aarch64-linux-gnu-g++
AR= aarch64-linux-gnu-ar
endif
#ifeq ($(platform), armhf)
#CC = arm-linux-gnueabihf-g++
#AR= arm-linux-gnueabihf-ar
#CFLAGS += -march=armv5
#LDLIB += -lrt
#endif

