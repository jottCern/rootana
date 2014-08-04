LIB := zsvanalysis
#TEST := test.exe
DICT := 1

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lra

all: plot_single_efficiency plot_mistag

include ../Makefile.rules
