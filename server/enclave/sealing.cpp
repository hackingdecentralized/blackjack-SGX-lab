#include <string>

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/seal.h>

#include "sealing.h"
#include "log.h"

int seal_data(int seal_policy, 
    const uint8_t * optional_message,
    size_t optional_message_size,
    const uint8_t* data, 
    size_t data_size, 
    message_t* sealed_data)
{
    int ret = -1;
    oe_result_t result;
    uint8_t* blob;
    size_t blob_size;
    sealed_data_t* temp_sealed_data;
    const oe_seal_setting_t settings[] = {OE_SEAL_SET_POLICY(seal_policy)};

    if (optional_message_size > sizeof(temp_sealed_data->optional_message)) {
          TRACE_ENCLAVE("optional message too large\n");
        goto exit;
    }

#ifndef OE_SIMULATION
    result = oe_seal(
        NULL,
        settings,
        sizeof(settings) / sizeof(*settings),
        data,
        data_size,
        optional_message,
        optional_message_size,
        &blob,
        &blob_size);
    if (result != OE_OK) {
          TRACE_ENCLAVE("oe_seal() failed with %s\n", oe_result_str(result));
        goto exit;
    }
#else 
    blob = (uint8_t *) data;
    blob_size = data_size;
#endif
    if (blob_size > UINT32_MAX) {
          TRACE_ENCLAVE("blob_size is too large to fit into an uint32\n");
        goto exit;
    }
    temp_sealed_data =
        (sealed_data_t*)oe_malloc(sizeof(*temp_sealed_data) + blob_size);
    if (temp_sealed_data == NULL) {
          TRACE_ENCLAVE("oe_malloc() out of memory\n");
        goto exit;
    }

    memset(temp_sealed_data, 0, sizeof(*temp_sealed_data));
    memcpy(
        temp_sealed_data->optional_message,
        optional_message,
        optional_message_size);
    temp_sealed_data->sealed_blob_size = blob_size;
    memcpy(temp_sealed_data + 1, blob, blob_size);
    sealed_data->data = (uint8_t*)temp_sealed_data;
    sealed_data->size = sizeof(*temp_sealed_data) + blob_size;
    ret = 0;
exit:
#ifndef OE_SIMULATION
    if (blob)
        oe_free(blob);
#endif
    return ret;
}

    
int unseal_data(const uint8_t* sealed_data,
    size_t sealed_data_size, 
    const uint8_t* optional_message,
    size_t optional_message_size,
    uint8_t** output_data, 
    size_t * output_data_size)
{
    oe_result_t result;
    int ret = -1;
    uint8_t* temp_data = nullptr;    
    size_t temp_data_size;
    sealed_data_t* unwrapped_sealed_data = (sealed_data_t*) sealed_data;
    if (unwrapped_sealed_data->sealed_blob_size + sizeof(*unwrapped_sealed_data) != sealed_data_size) {
          TRACE_ENCLAVE("seal data does not match the seal data size, expected %zd, got: %zd\n",
                unwrapped_sealed_data->sealed_blob_size + sizeof(*unwrapped_sealed_data), sealed_data_size);
        goto exit;
    }
#ifndef OE_SIMULATION
    result = oe_unseal(
        (const uint8_t*)(unwrapped_sealed_data + 1),
        unwrapped_sealed_data->sealed_blob_size,
        optional_message,
        optional_message_size,
        &temp_data,
        &temp_data_size);
    if (result != OE_OK) {
          TRACE_ENCLAVE("oe_unseal() returns %s\n", oe_result_str(result));
        goto exit;
    }
#else 
    if(memcmp(unwrapped_sealed_data->optional_message, optional_message, optional_message_size)!=0) {
          TRACE_ENCLAVE("optional message does not match\n");
        goto exit;
    }
    temp_data_size = unwrapped_sealed_data->sealed_blob_size;
    temp_data = (uint8_t *) oe_malloc(temp_data_size);
    if (temp_data == NULL) {
          TRACE_ENCLAVE("oe_malloc() out of memory\n");
        goto exit;
    }
    memcpy(temp_data, (uint8_t*)(unwrapped_sealed_data + 1), unwrapped_sealed_data->sealed_blob_size);
#endif

    *output_data = temp_data;
    *output_data_size = temp_data_size;
    ret = 0;

exit:
    return ret;
}
