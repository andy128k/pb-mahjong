POCKETBOOKSDK=/usr/local/pocketbook/FRSCSDK

SIM_CFLAGS=`pkg-config --cflags freetype2`

SRC=\
	src/bitmaps.c \
	src/board.c \
	src/common.c \
	src/geometry.c \
	src/main.c \
	src/maps.c \
	src/menu.c \
	src/messages.c \
	images-temp.c

all: pb-mahjong pb-mahjong.app

images-temp.c:
	$(POCKETBOOKSDK)/bin/pbres -c images-temp.c images/*.bmp

pb-mahjong: $(SRC)
	gcc -o pb-mahjong -m32 -g3 -Wall -DEMULATION=1 -DIVSAPP -I$(POCKETBOOKSDK)/include $(SIM_CFLAGS) $(SRC) -L$(POCKETBOOKSDK)/lib -Wl,-rpath $(POCKETBOOKSDK)/lib -linkview

pb-mahjong.app: $(SRC)
	$(POCKETBOOKSDK)/bin/arm-none-linux-gnueabi-gcc -o pb-mahjong.app -Wall -I$(POCKETBOOKSDK)/include $(SRC) -pthread -linkview -lfreetype -lz -lm
	$(POCKETBOOKSDK)/bin/arm-none-linux-gnueabi-strip pb-mahjong.app

clean:
	rm -f images-temp.* pb-mahjong pb-mahjong.app

