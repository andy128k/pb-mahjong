OUT = pb-mahjong
VPATH+= 

CFLAGS+= -DHAS_NO_IV_GET_DEFAULT_FONT

ifndef BUILD
BUILD=emu
endif


ifeq (${BUILD},emu)
CFLAGS+=`freetype-config --cflags`
endif

SOURCES = \
    src/bitmaps.c\
    src/board.c   \
    src/common.c   \
    src/geometry.c  \
    src/main.c  \
    src/maps.c   \
    src/menu.c    \
    src/messages.c


BITMAPS = \
    images/background.bmp\
    images/chip_51.bmp\
    images/chip_52.bmp\
    images/chip_53.bmp\
    images/chip_54.bmp\
    images/chip_55.bmp\
    images/chip_56.bmp\
    images/chip_57.bmp\
    images/chip_58.bmp\
    images/chip_59.bmp\
    images/chip_61.bmp\
    images/chip_62.bmp\
    images/chip_63.bmp\
    images/chip_64.bmp\
    images/chip_65.bmp\
    images/chip_66.bmp\
    images/chip_67.bmp\
    images/chip_68.bmp\
    images/chip_69.bmp\
    images/chip_71.bmp\
    images/chip_72.bmp\
    images/chip_73.bmp\
    images/chip_74.bmp\
    images/chip_75.bmp\
    images/chip_76.bmp\
    images/chip_77.bmp\
    images/chip_78.bmp\
    images/chip_79.bmp\
    images/chip_91.bmp\
    images/chip_92.bmp\
    images/chip_93.bmp\
    images/chip_94.bmp\
    images/chip_A1.bmp\
    images/chip_A2.bmp\
    images/chip_A3.bmp\
    images/chip_D1.bmp\
    images/chip_D2.bmp\
    images/chip_D3.bmp\
    images/chip_D4.bmp\
    images/chip_E1.bmp\
    images/chip_E2.bmp\
    images/chip_E3.bmp\
    images/chip_E4.bmp

include /usr/local/pocketbook/common.mk

$(OBJDIR):
	mkdir -p $(OBJDIR)/src
	mkdir -p $(OBJDIR)/images

