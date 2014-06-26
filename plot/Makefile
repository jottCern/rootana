LIB := plot
#TEST := test.exe

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lra -lbase

# Use Makefile.local to control what is compiled as default program
-include Makefile.local

default: plot

# all: plot_zbb_dominik plot_ivf plotzbb plot ../lib/libplot.so

include ../Makefile.rules

%: .bin/%.o ../lib/libplot.so
	$(EXE_CMD) -lplot
