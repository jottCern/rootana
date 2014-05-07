LIB := dra
TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lbase -lra -ldc

all: ../lib/libdra.so dra_local

include ../Makefile.rules

dra_local: .bin/dra_local.o
	$(EXE_CMD) -ldra