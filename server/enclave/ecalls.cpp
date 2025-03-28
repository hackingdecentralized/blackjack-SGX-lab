#include <iostream>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/enclave.h>

#include "common/shared.h"
#include "crypto.h"
#include "blackjack.h"
#include "blackjack_t.h"
#include "sealing.h"
#include "log.h"
#include "server.h"

const char * SEAL_SIGN_KEY_NAME = "signing_key";
const char * SEAL_CARD_MAPPING_NAME = "card_mapping";

oe_result_t load_oe_modules()
{
    oe_result_t result = OE_FAILURE;

    // Explicitly enabling features
    if ((result = oe_load_module_host_resolver()) != OE_OK)
    {
        TRACE_ENCLAVE(
            "oe_load_module_host_resolver failed with %s\n",
            oe_result_str(result));
        goto exit;
    }
    if ((result = oe_load_module_host_socket_interface()) != OE_OK)
    {
        TRACE_ENCLAVE(
            "oe_load_module_host_socket_interface failed with %s\n",
            oe_result_str(result));
        goto exit;
    }
    TRACE_ENCLAVE("enabled socket features\n");
exit:
    return result;
}

int generate_attestation_report(message_t* report) {
    uint8_t * private_key_buffer = nullptr;
    uint8_t * public_key_buffer = nullptr;
    size_t private_key_buffer_size;
    size_t public_key_buffer_size;
    message_t sealed_private_key;
    message_t public_key;
    oe_result_t result;
    uint8_t report_data[32] = {0};
    size_t report_data_size = sizeof(report_data);
    int ret = -1;
    int res, i;
    uint32_t flags = OE_REPORT_FLAGS_REMOTE_ATTESTATION;
    oe_report_t * parsed_report = nullptr;
    report->data = nullptr;

    res = generate_signing_key(&private_key_buffer, &private_key_buffer_size, &public_key_buffer, &public_key_buffer_size);
    if(res != 0) {
        TRACE_ENCLAVE( "generate_signing_key failed\n");
        goto exit;
    }
    TRACE_ENCLAVE("generated new signing key\n");
    seal_data(
        OE_SEAL_POLICY_UNIQUE,
        (const uint8_t *) SEAL_SIGN_KEY_NAME,
        strlen(SEAL_SIGN_KEY_NAME),
        (const uint8_t *) private_key_buffer,
        private_key_buffer_size,
        &sealed_private_key);
    TRACE_ENCLAVE("sealed private key\n");

    store_private_key(&res, &sealed_private_key);
    if(res != 0) {
        TRACE_ENCLAVE( "failed to store sealed private key\n");
        goto exit;
    }
    TRACE_ENCLAVE("stored sealed private key\n");

    public_key.data = public_key_buffer;
    public_key.size = public_key_buffer_size;
    store_public_key(&res, &public_key);
    if(res != 0) {
        TRACE_ENCLAVE( "failed to store public key\n");
        goto exit;
    }
    TRACE_ENCLAVE("stored plaintext public key\n");

    res = Sha256(public_key_buffer, public_key_buffer_size, report_data);
    if(res != 0) {
        TRACE_ENCLAVE( "Sha256 calculation failed\n");
        goto exit;
    }

    TRACE_ENCLAVE("requesting report with data SHA256(public key): ");
    for(i = 0; i < report_data_size; i++) {
          printf("%02x", report_data[i]);
    }
      printf("\n");

#ifndef OE_SIMULATION
    result = oe_get_report(flags, (const uint8_t *) report_data, 
                report_data_size, NULL, 0,  &report->data, &report->size);
#else 
    // create a report with reasonable values for testing purposes
    report->data = (uint8_t *) oe_malloc(sizeof(oe_report_t) + report_data_size);
    if (report->data == NULL) {
        TRACE_ENCLAVE("oe_malloc() out of memory\n");
        goto exit;
    }
    report->size = sizeof(oe_report_t) + report_data_size;
    parsed_report = (oe_report_t *) report->data;
    parsed_report->size = sizeof(oe_report_t);
    parsed_report->type = OE_ENCLAVE_TYPE_SGX;
    parsed_report->report_data_size = report_data_size;
    parsed_report->report_data = report->data + sizeof(oe_report_t);
    memcpy(parsed_report->report_data, report_data, report_data_size);
    parsed_report->enclave_report_size = report->size;
    parsed_report->enclave_report = report->data;
    parsed_report->identity = oe_identity_t{0, 0, OE_SIMULATION, NULL, NULL, NULL};
    memcpy(parsed_report->identity.unique_id, TEST_MRENCLAVE, OE_UNIQUE_ID_SIZE);
    memcpy(parsed_report->identity.signer_id, TEST_MRSIGNER, OE_SIGNER_ID_SIZE);
    memcpy(parsed_report->identity.product_id, TEST_APP_VERSION, OE_PRODUCT_ID_SIZE);
    result = OE_OK;
#endif

    if(OE_OK != result) {
        TRACE_ENCLAVE( "oe_get_report failed with %s\n", oe_result_str(result));
        goto exit;
    }
    ret = 0;
exit:
    if(private_key_buffer != NULL)
        oe_free(private_key_buffer);
    if(public_key_buffer != NULL)
        oe_free(public_key_buffer);

    return ret;
}


int setup_card_mapping() {
    int res;
    uint8_t file_sprite[FILE_SPRITE_SIZE] = {0};
    read_card_sprite(&res, file_sprite, sizeof(file_sprite));
    if (res != 0) {
        TRACE_ENCLAVE("failed to read card sprite\n");
        return res;
    }
    TRACE_ENCLAVE("read card sprite\n");

    int bytes_per_pixel = 4;
    uint32_t header_offset = *((uint32_t*) &file_sprite[BMP_OFFSET_LOC]);
    uint32_t width = *((uint32_t*) &file_sprite[BMP_WIDTH_LOC]);
    uint32_t height = *((uint32_t*) &file_sprite[BMP_HEIGHT_LOC]);
    uint32_t crop_height = 98;
    uint32_t crop_width = 73;

    if (crop_height != (height/4)) {
        TRACE_ENCLAVE("unexpected sprite height expected %d got %d\n", crop_height*4, height);
        return -1;
    }
    if (crop_width != (width/13)) {
        TRACE_ENCLAVE("unexpected sprite width expected %d got %d\n", crop_width*13, width);
        return -1;
    }

    uint32_t card_size = crop_height * crop_width * bytes_per_pixel + header_offset;
    if (card_size != CARD_SIZE) {
        TRACE_ENCLAVE("unexpected calculated card size expected %d got %d\n", CARD_SIZE, card_size);
        return -1;
    }

    uint8_t card[CARD_SIZE] = {0};

    //copy bmp header
    memcpy(card, file_sprite, header_offset);

    //fix header for card
    *((uint32_t*) &card[BMP_SIZE_LOC]) = card_size;
    *((uint32_t*) &card[BMP_WIDTH_LOC]) = crop_width;
    *((uint32_t*) &card[BMP_HEIGHT_LOC]) = crop_height;

    //shuffle card to filename mapping
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<int> card_order(52);
    std::iota (std::begin(card_order), std::end(card_order), 0); 
    std::shuffle(card_order.begin(), card_order.end(), gen);
    std::map<char, std::map<char, int>> card_mapping;
    int filename = 0;
    int x, y, card_x, card_y;
    for (const auto& i : card_order) {
        x = i % 13;
        y = i % 4;
        card_x = x*crop_width;
        card_y = y*crop_height;
        card_mapping[RANKS[x]][SUITS[y]] = filename;

        //crop out that card
        for (int row = 0; row < crop_height; row++) {
            for (int col = 0; col < crop_width; col++) {
                for (int channel = 0; channel < bytes_per_pixel; channel++) {
                    card[(row * crop_width + col) * bytes_per_pixel + channel + header_offset] = file_sprite[header_offset + ((card_y + row) * width + (card_x + col)) * bytes_per_pixel + channel];
                }
            }
        }

        char path[24] = {0};
        sprintf(path, "%02d.seal", filename);
        char card_name[2] = {RANKS[x], SUITS[y]};
        
        message_t sealed_data;
        int optional_msg_flag = 1;

        seal_data(
            OE_SEAL_POLICY_UNIQUE,
            (const uint8_t *) card_name,
            sizeof(card_name),
            (const uint8_t *) card,
            card_size,
            &sealed_data);
        
        store_card(path, &sealed_data);
        filename++;
    }
    std::string card_mapping_str;

    for (const auto& pair : card_mapping) {
        for (const auto& pair2 : pair.second) {
            card_mapping_str += pair.first;
            card_mapping_str += ",";
            card_mapping_str += pair2.first;
            card_mapping_str += ":";
            card_mapping_str += std::to_string(pair2.second);
            card_mapping_str += "\n";
        }
    }

    message_t sealed_card_mapping;
    TRACE_ENCLAVE("generated new card mapping\n");
    const char * card_mapping_cstr = card_mapping_str.c_str();
    size_t card_mapping_size = strlen(card_mapping_cstr)+1;
    res = seal_data(
        OE_SEAL_POLICY_UNIQUE,
        (const uint8_t *) SEAL_CARD_MAPPING_NAME,
        strlen(SEAL_CARD_MAPPING_NAME),
        (const uint8_t *) card_mapping_cstr,
        card_mapping_size,
        &sealed_card_mapping);
    if(res != 0) {
        TRACE_ENCLAVE( "failed to seal card mapping\n");
        return -1;
    }
    TRACE_ENCLAVE("sealed card mapping %lu\n", sealed_card_mapping.size);

    store_card_mapping(&res, &sealed_card_mapping);
    oe_free(sealed_card_mapping.data);
    if(res != 0) {
        TRACE_ENCLAVE( "failed to store sealed card mapping\n");
        return -1;
    }
    TRACE_ENCLAVE("stored sealed card mapping\n");

    return 0;
}

int load_card_mapping() {
    uint8_t sealed_card_mapping[SEAL_CARD_MAPPING_SIZE];
    uint8_t * card_mapping = nullptr;
    size_t card_mapping_size;
    int res; 
    int ret = -1;
    char suit;
    char rank;
    int mapping;
    int bytesRead, pos = 0;

    read_card_mapping(&res, sealed_card_mapping, sizeof(sealed_card_mapping));
    if (res != 0) {
        TRACE_ENCLAVE("failed to read card mapping\n");
        goto exit;
    }

    res = unseal_data(sealed_card_mapping, 
                        sizeof(sealed_card_mapping), 
                        (const uint8_t *) SEAL_CARD_MAPPING_NAME,
                        strlen(SEAL_CARD_MAPPING_NAME), 
                        &card_mapping, 
                        &card_mapping_size);

    if (res != 0) {
        TRACE_ENCLAVE("failed to unseal card mapping\n");
        goto exit;
    }

    for(int i = 0; i < 52; i++) {
        res = sscanf((const char *) card_mapping + pos , "%c,%c:%d\n%n", &rank, &suit, &mapping, &bytesRead);
        if (res != 3) {
            TRACE_ENCLAVE("failed to parse card mapping\n");
            goto exit;
        }
        CARD_MAPPING[rank][suit] = mapping;
        pos += bytesRead;
    }

    ret = 0;
exit:
    if (card_mapping != NULL) {
        oe_free(card_mapping);
    }
    return ret;
}

int run_server(int server_port_number) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int ret = -1;
    int addrlen = sizeof(address);
    int res;
    int num_games;
    size_t i;
    uint8_t sealed_signing_key[SEAL_PRIVATE_KEY_SIZE];

    size_t signing_key_size;
    uint8_t * signing_key = nullptr;
    message_t signature, games_won;
    char games_won_buffer[100] = {0};
    uint8_t * signature_buffer;
    size_t signature_buffer_size;

    /* Load host resolver and socket interface modules explicitly */
    if (load_oe_modules() != OE_OK)
    {
        TRACE_ENCLAVE("loading required Open Enclave modules failed\n");
        goto exit;
    }

    if (load_card_mapping() != OE_OK)
    {
        TRACE_ENCLAVE("unable to loaded encrypted card deck\n");
        goto exit;
    }

    TRACE_ENCLAVE("loaded encrypted card deck\n");

    if (create_listener_socket(server_port_number, server_fd) != 0)
    {
        TRACE_ENCLAVE("unable to create listener socket on the server\n");
        goto exit;
    }
    TRACE_ENCLAVE("created socket\n");

    num_games = handle_communication_until_done(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (num_games == -1) {
        TRACE_ENCLAVE("server failed");
        goto exit;
    }

    read_private_key(&res, sealed_signing_key, sizeof(sealed_signing_key));
    if (res != 0) {
        TRACE_ENCLAVE("failed to read signing key\n");
        goto exit;
    }

    res = unseal_data(sealed_signing_key, 
                        sizeof(sealed_signing_key), 
                        (const uint8_t *) SEAL_SIGN_KEY_NAME,
                        strlen(SEAL_SIGN_KEY_NAME), 
                        &signing_key, 
                        &signing_key_size);

    if (res != 0) {
        TRACE_ENCLAVE("failed to unseal signing key\n");
        goto exit;
    }

    if (signing_key_size != PRIVATE_KEY_SIZE) {
        TRACE_ENCLAVE("unexpected signing key size %lu!=%d\n", signing_key_size, PRIVATE_KEY_SIZE);
        return -1;
    }

    TRACE_ENCLAVE("read signing key\n");
    sprintf(games_won_buffer, "%s games_won: %u", userid.c_str() , num_games);
    TRACE_ENCLAVE("%s\n", games_won_buffer);

    res = sign_data(signing_key, SEAL_PRIVATE_KEY_SIZE, (uint8_t *) games_won_buffer, strlen(games_won_buffer), &signature_buffer, &signature_buffer_size);
    if (res != 0) {
        TRACE_ENCLAVE("failed to sign games_won message\n");
        goto exit;
    }
    TRACE_ENCLAVE("signed data\n");
    signature.data = signature_buffer;
    signature.size = signature_buffer_size;
    games_won.data = (uint8_t *) games_won_buffer;
    games_won.size = strlen(games_won_buffer);
    store_signature(&res, &games_won, &signature);
    if (res != 0) {
        TRACE_ENCLAVE("failed to store signature\n");
        goto exit;
    }
    ret = 0;
exit:
    if (signing_key != NULL) {
        oe_free(signing_key);
    }
    close(server_fd);
    return ret;
}
