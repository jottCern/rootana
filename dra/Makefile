LIB := dra
TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lbase -lra -ldc

all: dra_local  ../lib/libdra.so

include ../Makefile.rules

dra_local: .bin/dra_local.o
	$(EXE_CMD) -ldra