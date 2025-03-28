#ifndef _SEALING_H
#define _SEALING_H

#include <blackjack_t.h>

#define MAX_OPTIONAL_MESSAGE_SIZE 128
#define CARD_SIZE 28754
#define PRIVATE_KEY_SIZE 237

#ifndef OE_SIMULATION
#define SEAL_CARD_SIZE 29450
#define SEAL_PRIVATE_KEY_SIZE 933
#define SEAL_CARD_MAPPING_SIZE 1051
#else
#define SEAL_CARD_SIZE 28890
#define SEAL_PRIVATE_KEY_SIZE 373
#define SEAL_CARD_MAPPING_SIZE 491
#endif 

typedef struct _sealed_data_t
{
    uint8_t optional_message[MAX_OPTIONAL_MESSAGE_SIZE];
    size_t sealed_blob_size;
} sealed_data_t;

int seal_data(int seal_policy, 
    const uint8_t * optional_message,
    size_t optional_message_size,
    const uint8_t* data, 
    size_t data_size, 
    message_t* sealed_data);

int unseal_data(const uint8_t* sealed_data,
    size_t sealed_data_size, 
    const uint8_t* optional_message,
    size_t optional_message_size,
    uint8_t** output_data, 
    size_t * output_data_size);

#endif /* _SEALING_H */
