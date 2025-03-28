
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blackjack_u.h"

#include "files.h"

const char * seal_dir = "sealed_data/";
const char * private_key_path = "sealed_data/private_key.seal";
const char * card_mapping_path = "sealed_data/card_mapping.seal";


const char * games_won_path = "output_data/gameswon.txt";
const char * signature_path = "output_data/signature.data";
const char * public_key_path = "output_data/public_key.pem";

int read_card_mapping(uint8_t* data, size_t data_size) {
    printf("Host: reading sealed mapping\n");
    size_t size_read;
    read_file(card_mapping_path, "rb", data, data_size, &size_read);
    if (size_read != data_size) {
        printf("Host: unable to read full file expected %lu got %lu\n",size_read, data_size);
        return -1;
    }
    return 0;
}

int store_card_mapping(message_t* data) {
    printf("Host: writing sealed mapping\n");
    write_file(card_mapping_path, "w", data->data, data->size);
    return 0;   
}

int store_signature(message_t* data, message_t* signature) {
    write_file(games_won_path, "w", data->data, data->size);
    write_file(signature_path, "wb", signature->data, signature->size);
    return 0;   
}

int store_public_key(message_t* data) {
    write_file(public_key_path, "w", data->data, data->size);
    return 0;
}


int store_private_key(message_t* data) {
    printf("Host: writing sealed private key\n");
    write_file(private_key_path, "wb", data->data, data->size);
    return 0;
}

int read_private_key(uint8_t* data, size_t data_size) {
    size_t size_read;
    printf("Host: reading sealed private key\n");
    read_file(private_key_path, "rb", data, data_size, &size_read);
    if (size_read != data_size) {
        printf("unable to read full file expected %lu got %lu\n",size_read, data_size);
        return -1;
    }

    return 0;
}

void store_card(const char* path, message_t* data) {
    printf("Host: writing sealed card %s\n", path);
    std::string seal_path(seal_dir);
    seal_path.append(path);
    write_file(seal_path.c_str(), "wb", data->data, data->size);
}

int read_card(const char* path, uint8_t* data, size_t data_size) {
    printf("Host: reading sealed card %s\n", path);
    size_t size_read;
    std::string seal_path(seal_dir);
    seal_path.append(path);
    read_file(seal_path.c_str(), "rb", data, data_size, &size_read);
    return 0;
}

int read_card_sprite(uint8_t * data, size_t data_size) {
    size_t size_read;
    read_file("../images/cards.jfitz.bmp", "rb", data, data_size, &size_read);

    if (size_read != data_size) {
        printf("Host: unable to read full sprite file expected %lu got %lu\n",size_read, data_size);
        return -1;
    }
    return 0;
}
