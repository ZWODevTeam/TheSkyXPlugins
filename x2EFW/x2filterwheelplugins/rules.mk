ver = debug
platform = mac32
VERSION = 1.1.7
#20221104 - 1.1.7

CC = i686-pc-linux-gnu-g++
AR= ar
IsLin=1

ifeq ($(ver), debug)
DEFS = -D_LIN -D_DEBUG
CFLAGS = -fPIC -g $(DEFS)
else
DEFS = -D_LIN
CFLAGS = -fPIC -O3 $(DEFS)
endif

ifeq ($(platform), mac32)
IsLin=0
CC = g++
CFLAGS += -D_MAC
CFLAGS += -m32

CFLAGS += -framework IOKit -framework foundation
endif

ifeq ($(platform), mac64)
IsLin=0
CC = g++
CFLAGS += -D_MAC
CFLAGS += -m64
endif

ifeq ($(platform), mac)
IsLin=0
CC = g++
CFLAGS += -D_MAC
CFLAGS += -arch i386 -arch x86_64 -framework IOKit -framework foundation
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
UDEV=-ludev -L ./libudev/$(platform)/
endif

