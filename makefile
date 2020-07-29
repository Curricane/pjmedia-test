CC = gcc
CFLAGS = -g -Wall -O
INCLUDE = -I./include
Libs =  -lpjsua2-x86_64-apple-darwin19.6.0 \
	-lstdc++ -lpjsua-x86_64-apple-darwin19.6.0 \
	-lpjsip-ua-x86_64-apple-darwin19.6.0 \
	-lpjsip-simple-x86_64-apple-darwin19.6.0 \
	-lpjsip-x86_64-apple-darwin19.6.0 \
	-lpjmedia-codec-x86_64-apple-darwin19.6.0 \
	-lpjmedia-x86_64-apple-darwin19.6.0 \
	-lpjmedia-videodev-x86_64-apple-darwin19.6.0 \
	-lpjmedia-audiodev-x86_64-apple-darwin19.6.0 \
	-lpjmedia-x86_64-apple-darwin19.6.0 \
	-lpjnath-x86_64-apple-darwin19.6.0 \
	-lpjlib-util-x86_64-apple-darwin19.6.0  \
	-lsrtp-x86_64-apple-darwin19.6.0 \
	-lresample-x86_64-apple-darwin19.6.0 \
	-lgsmcodec-x86_64-apple-darwin19.6.0 \
	-lspeex-x86_64-apple-darwin19.6.0 \
	-lilbccodec-x86_64-apple-darwin19.6.0 -lg7221codec-x86_64-apple-darwin19.6.0 \
	-lyuv-x86_64-apple-darwin19.6.0 -lwebrtc-x86_64-apple-darwin19.6.0  \
	-lpj-x86_64-apple-darwin19.6.0 -lopus -lm -lpthread  -framework CoreAudio \
	-framework CoreServices -framework AudioUnit -framework AudioToolbox \
	-framework Foundation -framework AppKit -framework AVFoundation \
	-framework CoreGraphics -framework QuartzCore -framework CoreVideo \
	-framework CoreMedia -framework VideoToolbox  -lSDL2   -framework Security
LIBPATH = -L./lib

BIN = simpleua auddemo auddemo_w

OBJ1 = simpleua.o 
SRC1 = ./src/simpleua.c 
BIN1 = simpleua

OBJ2 = auddemo.o 
SRC2 = ./src/auddemo.c 
BIN2 = auddemo

OBJ3 = auddemo_w.o 
SRC3 = ./src/auddemo_w.c 
BIN3 = auddemo_w

all: $(BIN)

$(BIN1):$(SRC1)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN1) $(SRC1) $(Libs) $(LIBPATH) 
$(BIN2):$(SRC2)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN2) $(SRC2) $(Libs) $(LIBPATH) 
$(BIN3):$(SRC3)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN3) $(SRC3) $(Libs) $(LIBPATH) 

clean:
	rm -rf *.dSYM
	rm -rf $(BIN)