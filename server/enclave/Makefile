# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

include ../config.mk

ifeq ($(OE_CRYPTO_LIB),openssl_3)
	CFLAGS=$(shell pkg-config oeenclave-$(C_COMPILER) --variable=${OE_CRYPTO_LIB}flags)
	CXXFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --variable=${OE_CRYPTO_LIB}flags)
else
	CFLAGS=$(shell pkg-config oeenclave-$(C_COMPILER) --cflags)
	CXXFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --cflags)
endif
ifeq ($(OE_SIMULATION),1)
	CFLAGS += "-DOE_SIMULATION"
	CXXFLAGS += "-DOE_SIMULATION"
endif

LDFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oeenclave-$(C_COMPILER) --variable=includedir)
CRYPTO_LDFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --variable=${OE_CRYPTO_LIB}libs)
LIBDIR=$(shell pkg-config oeenclave-$(C_COMPILER) --variable=libdir)/openenclave

all:
	$(MAKE) genkey
	$(MAKE) build
	$(MAKE) sign

mrsigner_private.pem:
	openssl genrsa -out $@ -3 3072

mrsigner_public.pem: mrsigner_private.pem
	openssl rsa -in $< -out $@ -pubout

genkey: mrsigner_public.pem

OE_SEAL_PLUGINS?= gcmaes
OE_SDK_BUILD_TYPES?= Release RelWithDebInfo MinSizeRel Debug

# Step 1 - Determine BUILD_TYPE (hence the directory containing seal plug-ins)
SEAL_PLUGIN_DIR:= $(or $(firstword $(wildcard $(addsuffix /,	\
	$(addprefix $(LIBDIR)/enclave/objects-,$(OE_SDK_BUILD_TYPES))))),	\
	$(error Cannot determine CMAKE_BUILD_TYPE. Object paths:	\
		$(shell find $(LIBDIR)/enclave -type d -name objects-*)))

# Step 2 - Locate seal plug-in object files.
SEAL_PLUGINS:= $(foreach p,$(OE_SEAL_PLUGINS),$(or	\
	$(wildcard $(SEAL_PLUGIN_DIR)oeseal_$p/*.o),	\
	$(wildcard $(SEAL_PLUGIN_DIR)oeseal_$p-lvi-cfg/*.o),	\
	$(error Seal plug-in \'$p\' missing in $(SEAL_PLUGIN_DIR))))


build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../blackjack.edl --trusted --trusted-dir ./ \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CXX) -g -c $(CXXFLAGS) $(INCLUDES) -I../../../include -I. -I.. -std=c++11 -DOE_API_VERSION=2 ecalls.cpp sealing.cpp server.cpp blackjack.cpp crypto.cpp 
	$(CC) -g -c $(CFLAGS) $(CINCLUDES) -I.. -DOE_API_VERSION=2 blackjack_t.c
	$(CXX) -o blackjack_enclave crypto.o ecalls.o sealing.o server.o blackjack.o blackjack_t.o  $(SEAL_PLUGINS) $(LDFLAGS) $(CRYPTO_LDFLAGS) -loehostsock -loehostresolver

sign:
	oesign sign -e blackjack_enclave -c enc.conf -k mrsigner_private.pem

clean:
	rm -f *.o blackjack_enclave blackjack_enclave.signed *_t.* *_args.h *.pem .*.pem
