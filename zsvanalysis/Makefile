LIB := zsvanalysis
#TEST := test.exe
DICT := 1

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS)
USERLDFLAGS += $(ROOT_LDFLAGS) -lra -lbase

BIN:=plot_reco plot_me plot_single_efficiency
#plot_gen 

include ../Makefile.rules
