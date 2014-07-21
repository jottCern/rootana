LIB := dra
TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lbase -lra -ldc

all: ../lib/libdra.so dra_local dra_worker dra_master

include ../Makefile.rules

dra_local: .bin/dra_local.o
	$(EXE_CMD) -ldra

dra_worker: .bin/dra_worker.o
	$(EXE_CMD) -ldra

dra_master: .bin/dra_master.o
	$(EXE_CMD) -ldra
