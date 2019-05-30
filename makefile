CC = g++

INSTALL_DIR = ./bin

LIB_DIR = -L ./lib ./lib/libliveMedia.a ./lib/libgroupsock.a ./lib/libBasicUsageEnvironment.a ./lib/libUsageEnvironment.a
INCLUDES = -I ./include -I ./include/BasicUsageEnvironment -I ./include/groupsock -I ./include/liveMedia -I ./include/UsageEnvironment

CFLAGS = -Wall -ggdb -std=c++11

LIBS = $(LIB_DIR) -Wl,-rpath=./:./lib

OBJS = src/YjTester.o src/YjMutex.o src/YjAudioFileSink.o src/YjH264VideoFileSink.o src/YjMpeg4FileSink.o src/YjOtherFileSink.o src/YjRTSPMedia.o 

TARGET = YjTester

all:$(TARGET)

YjTester:$(OBJS)
	$(CC) -o $@ $^ $(INCLUDES) $(LIBS)
%.o:%.c 
	$(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS)
%.o:%.cpp
	$(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS)
.PHONY:
clean:
	rm -f $(TARGET) src/*.o
install:$(TARGET)
	cp $(TARGET) $(INSTALL_DIR)
