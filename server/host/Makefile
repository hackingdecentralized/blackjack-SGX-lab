# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

include ../config.mk

CFLAGS=$(shell pkg-config oehost-$(C_COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(C_COMPILER) --variable=includedir)

ifeq ($(OE_SIMULATION),1)
	CFLAGS += "-DOE_SIMULATION"
	CXXFLAGS += "-DOE_SIMULATION"
endif

all: build

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../blackjack.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CC) -g -c $(CFLAGS) $(CINCLUDES) blackjack_u.c
	$(CXX) -g -c $(CXXFLAGS) $(INCLUDES) -I.. -std=c++11 cli.cpp ocalls.cpp files.cpp crypto.cpp
	$(CXX) -o cli cli.o ocalls.o crypto.o files.o blackjack_u.o $(LDFLAGS)

clean:
	rm -f cli *.o *_u.*  *_args.h
