include /usr/local/etc/PcapPlusPlus.mk

NDPI_INCLUDES= -I "/usr/include/ndpi/"
NDPI_LIBS= /usr/lib/libndpi.a

# All Target
all:
	g++ $(PCAPPP_INCLUDES) $(NDPI_INCLUDES) -c -o main.o main.cpp
	g++ $(PCAPPP_LIBS_DIR) -static-libstdc++ -o main main.o $(PCAPPP_LIBS) $(NDPI_LIBS)


# Clean Target
clean:
	rm main.o
	rm main