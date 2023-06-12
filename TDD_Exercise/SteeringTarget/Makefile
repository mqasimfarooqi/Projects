CC = g++
CFLAGS = -std=c++14 -Wall
LDFLAGS = -pthread

LIBS_DIR = /usr/local/lib

# Source files
SRCS = ../Generic/Generic.cpp SteeringTarget.cpp SteeringTarget_Test.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
EXEC = SteeringTarget

# Google Test libraries
GTEST_LIB = $(LIBS_DIR)/libgtest.a
GTEST_MAIN_LIB = $(LIBS_DIR)/libgtest_main.a

# PcapPlusPlus libraries
PACKET_LIB = $(LIBS_DIR)/libPacket++.a
PCAP_LIB = $(LIBS_DIR)/libPcap++.a
COMMON_LIB = $(LIBS_DIR)/libCommon++.a

# Include directories
INC_DIRS = -I/usr/local/include/pcapplusplus -I/usr/local/include/gtest -I/usr/local/include/gmock

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(GTEST_LIB) $(PACKET_LIB) $(PCAP_LIB) $(COMMON_LIB) $(GTEST_MAIN_LIB) -o $(EXEC)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC_DIRS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)