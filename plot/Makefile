LIB := plot
#TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lra -lbase

all: plot_zbb_dominik plot_ivf plotzbb plot ../lib/libplot.so

include ../Makefile.rules

plot: .bin/plot.o ../lib/libplot.so
	$(EXE_CMD) -lplot

plotzbb: .bin/plotzbb.o ../lib/libplot.so
	$(EXE_CMD) -lplot

plot_ivf: .bin/plot_ivf.o ../lib/libplot.so
	$(EXE_CMD) -lplot

plot_zbb_dominik: .bin/plot_zbb_dominik.o ../lib/libplot.so
	$(EXE_CMD) -lplot