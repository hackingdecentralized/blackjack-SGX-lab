// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include "crypto.h"
#include <openenclave/enclave.h>
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdlib.h>
#include <string.h>

int Sha256(const uint8_t* data, size_t data_size, uint8_t sha256[32])
{
    int ret = -1;
    EVP_MD_CTX* ctx = nullptr;

    if (!(ctx = EVP_MD_CTX_new()))
    {
        printf("EVP_MD_CTX_new failed!");
        goto exit;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL))
    {
        printf("EVP_DigestInit_ex failed!");
        goto exit;
    }

    if (!EVP_DigestUpdate(ctx, data, data_size))
    {
        printf("EVP_DigestUpdate failed!");
        goto exit;
    }

    if (!EVP_DigestFinal_ex(ctx, sha256, NULL))
    {
        printf("EVP_DigestFinal_ex failed!");
        goto exit;
    }

    ret = 0;
exit:
    if (ctx)
        EVP_MD_CTX_free(ctx);
    return ret;
}


int generate_signing_key(uint8_t ** private_key_buffer, size_t * private_key_buffer_size,
    uint8_t ** public_key_buffer, size_t * public_key_buffer_size) {
    int ret = -1;
    int res;

    BIO* mem = nullptr;
    size_t numbytes = 0;
    char* bio_ptr = nullptr;

    EVP_PKEY * ecdsa_key = nullptr;
    EVP_PKEY_CTX* ctx = nullptr;

    OpenSSL_add_all_algorithms();

    if(!(ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr))) {
        printf("EVP_PKEY_CTX_new_id failed!\n");
        goto exit;
    }
    
    if (!EVP_PKEY_keygen_init(ctx))
    {
        printf("EVP_PKEY_keygen_init failed!\n");
        goto exit;
    }


    if(!(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_secp256k1))) {
        printf("EC_KEY_generate_key failed!\n");
        goto exit;
    }
    
    if(!( EVP_PKEY_keygen(ctx, &ecdsa_key))) {
        printf("EVP_PKEY_keygen_init failed!\n");
        goto exit;
    }

    mem = BIO_new(BIO_s_mem());
    if (mem == NULL)
    {
        printf("BIO_new failed!\n");
        goto exit;
    }
    res = PEM_write_bio_PUBKEY(mem, ecdsa_key);
    if (res == 0)
    {
        printf("PEM_write_bio_PUBKEY failed!\n");
        goto exit;
    }
    numbytes = (size_t)BIO_get_mem_data(mem, &bio_ptr);
    if (numbytes == 0)
    {
        printf("BIO_get_mem_data failed!\n");
        goto exit;
    }
    *public_key_buffer = (uint8_t *) malloc(numbytes);
    if (public_key_buffer == nullptr)
    {
        ret = OE_OUT_OF_MEMORY;
        printf("copying public_key_buffer failed, out of memory");
        goto exit;
    }
    *public_key_buffer_size = numbytes;
    memcpy(*public_key_buffer, bio_ptr, numbytes);
    
    BIO_free(mem);
    mem = nullptr;
    mem = BIO_new(BIO_s_mem());
    if (mem == NULL)
    {
        printf("BIO_new failed!\n");
        goto exit;
    }
    res = PEM_write_bio_PrivateKey(mem, ecdsa_key, NULL , NULL, 0, 0, NULL);//todo check return type?
    if (res == 0)
    {
        printf("PEM_write_bio_PrivateKey failed!\n");
        goto exit;
    }
    numbytes = (size_t)BIO_get_mem_data(mem, &bio_ptr);
    if (numbytes == 0)
    {
        printf("BIO_get_mem_data failed!\n");
        goto exit;
    }
    *private_key_buffer = (uint8_t *) malloc(numbytes);
    if (*private_key_buffer == nullptr)
    {
        ret = OE_OUT_OF_MEMORY;
        printf("copying private_key_buffer failed, out of memory");
        goto exit;
    }
    *private_key_buffer_size = numbytes;
    memcpy(*private_key_buffer, bio_ptr, numbytes);

    ret = 0;

exit:
    if (mem)
        BIO_free(mem);
    if (ctx)
        EVP_PKEY_CTX_free(ctx);
    if (ecdsa_key)
        EVP_PKEY_free(ecdsa_key);
    return ret;
}

// Function to sign data using ECDSA
int sign_data(uint8_t * private_key_buffer, size_t private_key_buffer_size, uint8_t * data, size_t data_size, uint8_t **signature, size_t * signature_size) {
    BIO* mem = nullptr;
    size_t siglen;
    EVP_PKEY* private_key = nullptr;
    EVP_MD_CTX* mdctx = nullptr;
    int ret = 0;

    mem = BIO_new_mem_buf(private_key_buffer, private_key_buffer_size);
    if (mem == NULL)
    {
        printf("BIO_new failed!\n");
        ret = -1;
        goto exit;
    }

    PEM_read_bio_PrivateKey(mem, &private_key, NULL, NULL);
    if (private_key == nullptr) {
        printf("PEM_read_bio_PrivateKey failed!\n");
        ret = -1;
        goto exit;
        
    }

    mdctx = EVP_MD_CTX_new();
    if (!mdctx){
        printf("EVP_MD_CTX_new failed\n");
        ret = -1;
        goto exit;
    }

    if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) != 1) {
        printf("EVP_DigestSignInit failed\n");
        ret = -1;
        goto exit;
    }

    if (EVP_DigestSign(mdctx, nullptr, &siglen, data, data_size) != 1) {
        printf("EVP_DigestSign (size) failed\n");
        ret = -1;
        goto exit;
    }

    *signature = (uint8_t *) malloc(siglen);
    if (*signature == nullptr)
    {
        ret = -1;
        goto exit;
    }

    if (EVP_DigestSign(mdctx, *signature, &siglen, data, data_size) != 1){
        printf("EVP_DigestSign (sign) failed\n");
        ret = -1;
        free(*signature);
        goto exit;
    }

    *signature_size = siglen;

exit:
    if(mdctx)
        EVP_MD_CTX_free(mdctx);
    if (mem)
        BIO_free(mem);
    if (private_key)
        EVP_PKEY_free(private_key);
    return ret;
}
