#include <openssl/evp.h>
#include <openssl/pem.h>
#include "crypto.h"

bool Sha256(const uint8_t* data, size_t data_size, uint8_t sha256[HASH_SIZE])
{
    int ret = false;
    EVP_MD_CTX* ctx = nullptr;

    if (!(ctx = EVP_MD_CTX_new()))
    {
        printf("Host: EVP_MD_CTX_new failed!\n");
        goto exit;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL))
    {
        printf("Host: EVP_DigestInit_ex failed!\n");
        goto exit;
    }

    if (!EVP_DigestUpdate(ctx, data, data_size))
    {
        printf("Host: EVP_DigestUpdate failed!\n");
        goto exit;
    }

    if (!EVP_DigestFinal_ex(ctx, sha256, NULL))
    {
        printf("Host: EVP_DigestFinal_ex failed!\n");
        goto exit;
    }

    ret = true;
exit:
    if (ctx)
        EVP_MD_CTX_free(ctx);
    return ret;
}

// Function to verify an ECDSA signature
int verify_signature(const uint8_t * public_key_buffer, size_t public_key_buffer_size, const uint8_t * data, size_t data_size, const uint8_t *signature, size_t signature_size) {
    EVP_PKEY* public_key = nullptr;
    EVP_MD_CTX* mdctx = nullptr;
    BIO* mem = nullptr;
    int ret;
    mem = BIO_new_mem_buf(public_key_buffer, public_key_buffer_size);
    if (mem == NULL)
    {
        printf("Host: BIO_new failed!\n");
        ret = -1;
        goto exit;
    }

    PEM_read_bio_PUBKEY(mem, &public_key, NULL, NULL);
    if (public_key == nullptr) {
        printf("Host: PEM_read_bio_PUBKEY failed!\n");
        ret = -1;
        goto exit;
    }

    mdctx = EVP_MD_CTX_new();
    if (!mdctx){
        printf("Host: EVP_MD_CTX_new failed\n");
        ret = -1;
        goto exit;
    }
    
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) != 1) {
        printf("Host: EVP_DigestVerifyInit failed\n");
        ret = -1;
        goto exit;
    };

    if (EVP_DigestVerify(mdctx, signature, signature_size, data, data_size) != 1) {
        printf("Host: EVP_DigestVerify failed\n");
        ret = -1;
        goto exit;
    }
    ret = 0;
exit:
    if(mdctx)
        EVP_MD_CTX_free(mdctx);
    return ret;
}