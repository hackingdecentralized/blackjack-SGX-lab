#ifndef _CRYPTO_HOST_H
#define _CRYPTO_HOST_H

#define HASH_SIZE 32
bool Sha256(const uint8_t* data, size_t data_size, uint8_t sha256[HASH_SIZE]);
int verify_signature(const uint8_t * public_key_buffer, size_t public_key_buffer_size, const uint8_t * data, size_t data_size, const uint8_t *signature, size_t signature_size);

#endif /* _CRYPTO_HOST_H */