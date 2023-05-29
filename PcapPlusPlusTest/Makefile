include /usr/local/etc/PcapPlusPlus.mk

# All Target
all:
	g++ $(PCAPPP_INCLUDES) -g -c -o main.o main.cpp
	g++ $(PCAPPP_LIBS_DIR) -g -static-libstdc++ -o main main.o $(PCAPPP_LIBS)

# Clean Target
clean:
	rm main.o
	rm main