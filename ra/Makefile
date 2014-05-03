LIB := ra
TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lbase

all: ../lib/libra.so test.exe ra

include ../Makefile.rules

ra: .bin/ra.o ../lib/libra.so
	$(EXE_CMD) -lra
