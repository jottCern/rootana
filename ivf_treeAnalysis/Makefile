LIB := ivfanalysis
#TEST := test.exe
DICT := 1

USERCXXFLAGS += $(ROOT_CFLAGS) $(BOOST_CFLAGS) -I /cvmfs/cms.cern.ch/slc6_amd64_gcc472/cms/cmssw/CMSSW_6_2_5/src
USERLDFLAGS += $(ROOT_LDFLAGS) -lra
GENREFLEX_FLAGS = -I /cvmfs/cms.cern.ch/slc6_amd64_gcc472/cms/cmssw/CMSSW_6_2_5/src

#all: ../lib/libdra.so test.exe dra_local

include ../Makefile.rules
