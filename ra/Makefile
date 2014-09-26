LIB := ra
TEST := test.exe
BIN := ra

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lbase

include ../Makefile.rules

ra: .bin/ra.o ../lib/libra.so
	$(EXE_CMD) -lra

