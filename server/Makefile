# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

.PHONY: all build clean run

OE_CRYPTO_LIB := openssl_3
export OE_CRYPTO_LIB

OE_SIMULATION ?= 1

HOSTPORT := $(shell grep "^$(USER):" ../netid_ports.txt | cut -d ':' -f2)

ifeq ($(HOSTPORT),)
$(error No port found for user '$(USER)'. Please contact instructor.)
endif

all: build

build:
	mkdir -p sealed_data
	mkdir -p output_data

	$(MAKE) -C enclave OE_SIMULATION=$(OE_SIMULATION)
	$(MAKE) -C host

clean:
	$(MAKE) -C enclave clean
	$(MAKE) -C host clean
	rm -rf sealed_data
	rm -rf output_data

runsim:
	@echo "connect your client to port $(HOSTPORT)"
	OE_SIMULATION=1 host/cli run -mode:sim -enclave:./enclave/blackjack_enclave.signed -port:$(HOSTPORT)

runhw:
	@echo "connect your client to port $(HOST_PORT)"
	host/cli run -mode:hw -enclave:./enclave/blackjack_enclave.signed -port:$$HOST_PORT 
	
verifysim:
	host/cli verify -mode:sim

verifyhw:
	host/cli verify -mode:hw -mrenclave:0xd1d7d408e6aea4e9609eeae14278b89ca4973d63a0e77e704af3e19b77027ba0
