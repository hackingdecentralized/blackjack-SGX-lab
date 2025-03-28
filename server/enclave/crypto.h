// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef CRYPTO_H
#define CRYPTO_H

#include <openenclave/enclave.h>
#include <openssl/evp.h>
// Includes for mbedtls shipped with oe.
// Also add the following libraries to your linker command line:
// -loeenclave -lmbedcrypto -lmbedtls -lmbedx509

int Sha256(const uint8_t* data, size_t data_size, uint8_t sha256[32]);

int generate_signing_key(uint8_t ** private_key_buffer, size_t * private_key_buffer_size,
  uint8_t ** public_key_buffer, size_t * public_key_buffer_size);

int sign_data(uint8_t * private_key_buffer, size_t private_key_buffer_size, uint8_t * data, size_t data_size, uint8_t **signature, size_t * signature_size);

#endif // CRYPTO_H
