LIB := uhh2

ifeq ($(SFRAME_DIR),)
 $(error setup sframe before compiling)
endif

ifeq ($(CMSSW_BASE),)
 $(error setup cmssw before compiling)
endif

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS) -I$(CMSSW_BASE)/src
USERLDFLAGS += $(ROOT_LDFLAGS) -lra -lbase -L$(SFRAME_DIR)/lib -lSUHH2core -lProof -lTreePlayer -lPyROOT

include ../Makefile.rules
