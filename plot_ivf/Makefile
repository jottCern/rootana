LIB := plot
#TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lra -lbase

all: plot ../lib/libplot.so

include ../Makefile.rules

plot: .bin/plot.o ../lib/libplot.so
	$(EXE_CMD) -lplot
