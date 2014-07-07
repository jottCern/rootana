LIB := unfold
TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lMathMore -lbase

all: make-test-input unf plot

include ../Makefile.rules

unf: .bin/unf.o ../lib/libunfold.so
	@$(EXE_CMD) -lunfold -lra

make-test-input: .bin/make-test-input.o ../lib/libunfold.so
	@$(EXE_CMD) -lunfold
	
plot: .bin/plot.o ../lib/libunfold.so
	@$(EXE_CMD) -lunfold -lplot -lra
