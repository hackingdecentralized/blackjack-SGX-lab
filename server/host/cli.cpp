#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <openenclave/attestation/custom_claims.h>
#include <openenclave/attestation/sgx/evidence.h>
#include <openenclave/attestation/sgx/report.h>
#include <openenclave/attestation/verifier.h>
#include <openenclave/host.h>

#include "common/shared.h"
#include "crypto.h"
#include "files.h"
#include "blackjack_u.h"


oe_enclave_t* create_enclave(const char* enclave_path, uint32_t flags)
{
    oe_enclave_t* enclave = NULL;

    oe_result_t result = oe_create_blackjack_enclave(
        enclave_path, OE_ENCLAVE_TYPE_AUTO, flags, NULL, 0, &enclave);

    if (result != OE_OK)
    {
        printf(
            "Host: oe_create_blackjack_enclave failed. %s\n",
            oe_result_str(result));
    }
    else
    {
        printf("Host: Enclave successfully created.\n");
    }
    return enclave;
}

void terminate_enclave(oe_enclave_t* enclave)
{
    oe_terminate_enclave(enclave);
    printf("Host: Enclave successfully terminated.\n");
}


int verify_attestation(
    int hw_mode, uint8_t * report, 
    size_t report_size, 
    uint8_t * public_key_data, 
    size_t public_key_data_size,
    uint8_t expected_mrenclave[OE_UNIQUE_ID_SIZE],
    int check_mrenclave) 
{
    oe_report_t parsed_report_data;
    oe_report_t * parsed_report = nullptr;
    uint8_t hash[HASH_SIZE];
    oe_result_t result;

    if (hw_mode == 1) {
        result = oe_verify_report(NULL, report, report_size, &parsed_report_data);
        parsed_report = &parsed_report_data;
        if (result != OE_OK) {
            printf(
                "Host: oe_verify_report failed. %s\n",
                oe_result_str(result));
            return -1;
        }
        printf("Host: mrenclave: 0x");
        for(size_t i = 0; i < OE_UNIQUE_ID_SIZE; i++)
            printf("%02x", parsed_report->identity.unique_id[i]);
        printf("\n");
        if (check_mrenclave == 1) {
            if (memcmp(parsed_report->identity.unique_id, expected_mrenclave, OE_UNIQUE_ID_SIZE) != 0) {
                printf("Host: Mrenclave does not match expected value: 0x");
                for(size_t i = 0; i < OE_UNIQUE_ID_SIZE; i++)
                    printf("%02x", expected_mrenclave[i]);
                printf("\n");
                return -1;
            }
        }
    } else {
        parsed_report = (oe_report_t *) report;
        parsed_report->report_data = report + sizeof(oe_report_t);
        printf("Host: ignoring -mrenclave option for simulation mode\n");
        if (memcmp(parsed_report->identity.unique_id, TEST_MRENCLAVE, OE_UNIQUE_ID_SIZE) != 0) {
            printf("Host: Mrenclave does not match expected test value\n");
            return -1;
        }
    }

    printf("Host: got report data: ");
    for(int i =0; i < parsed_report->report_data_size; i++) {
        printf("%02x", parsed_report->report_data[i]);
    }
    printf("\n");

    if (Sha256((const uint8_t*) public_key_data, public_key_data_size, hash) != true)
    {
        printf("Host: failed to calculate public_key hash\n");
        return -1;
    }
    printf("Host: expected report data: ");
    for(int i =0; i < sizeof(hash); i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    if (memcmp(parsed_report->report_data, hash, sizeof(hash)) != 0) {
        printf("Host: report data inconsistant\n");
        return -1;
    }
    return 0;
}

int generate_attestation(
    int hw_mode,
    oe_enclave_t* attester_enclave)
{
    oe_result_t result = OE_OK;
    int res;
    int ret = 1;
    message_t report = {0};
    report.data = nullptr;
    uint8_t public_key_data[174];
    size_t public_key_data_size;

    printf("Host: Requesting attestation report\n");
    result = generate_attestation_report(attester_enclave, &res, &report);

    if ((result != OE_OK) || (res != 0))
    {
        printf(
            "Host: get_attestation_report failed. %s\n",
            oe_result_str(result));
        goto exit;
    }
    read_file("output_data/public_key.pem", "r", public_key_data, sizeof(public_key_data), &public_key_data_size);
    if (sizeof(public_key_data) != public_key_data_size) {
        printf("Host: unable to read full public_key.pem file expected %lu got %lu\n",sizeof(public_key_data), public_key_data_size);
        goto exit;
    }
    printf("Host: verifying attestation report\n");
    verify_attestation(hw_mode, report.data, report.size, public_key_data, public_key_data_size, NULL, 0);
    if (write_file("output_data/report.data", "wb", report.data, report.size) != 0) {
        printf("Host: failed to save report\n");
        goto exit;
    }
    printf("Host: saved report to output_data/report.data\n");
    ret = 0;
exit:
    if(report.data != NULL)
        free(report.data);
    return ret;
}


void print_usage(const char * name) {
    printf("Available Commands:\n");
    printf("  run: run a server and generate verifiable output\n");
    printf("    Usage: %s run -mode:<sim or hw> -enclave:<enclave_path> -port:<port> \n", name);
    printf("    Options:\n");
    printf("      -mode\n");
    printf("        sim: run in simulation mode without fake attestation generation or sealing\n");
    printf("        hw: run in hardware mode with remote attestation generation and sealing\n");
    printf("      -enclave: path to signed enclave .so file\n");
    printf("      -port: port to run server on\n\n");
    printf("  verify: verify output from server\n");
    printf("    Usage: %s verify -mode:<sim or hw> -mrenclave:<mrenclave>\n", name);
    printf("    Options:\n");
    printf("      -mode:\n");
    printf("        sim: run in simulation mode without real attestation checking\n");
    printf("        hw: run in hardware mode with remote attestation check\n");
    printf("      -mrenclave: 0x preceded 32 byte expected code hash for remote attestation report\n");
}


int new_game(int argc, const char* argv[]) {
    oe_enclave_t* enclave_ptr = NULL;
    int ret = 1;
    int res;
    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    int server_port = 0;
    const char* port_option = "-port:";
    const char * mode_option_sim = "-mode:sim";
    const char * mode_option_hw = "-mode:hw";
    const char* path_option = "-enclave:";
    const char* enclave_path = nullptr;
    size_t i;
    int j;
    int hw_mode = 0;

    for (int j = 2; j < argc; j++) {
        if (strncmp(argv[j], mode_option_sim, strlen(mode_option_sim)) == 0) {
            printf("Host: Using simulation mode\n");
            flags = OE_ENCLAVE_FLAG_SIMULATE;
            hw_mode = 2;
        } else if (strncmp(argv[j], mode_option_hw, strlen(mode_option_sim)) == 0) {
            printf("Host: Using hardware mode\n");
            hw_mode = 1;
        } else if (strncmp(argv[j], port_option, strlen(port_option)) == 0) {
            server_port = atoi((char*)(argv[j] + strlen(port_option)));
        } else if (strncmp(argv[j], path_option, strlen(path_option)) == 0) {
            enclave_path = argv[j] + strlen(path_option);
        }
    }
    if (hw_mode == 0) {
        printf("specify -mode:<hw or sim>\n");
        return -1;
    }
    if (enclave_path == NULL) {
        printf("specify -encalve:<path so signed enclave .so>\n");
        return -1;
    }
    if (server_port == 0) {
        printf("specify -port:<port to run server on>\n");
        return -1;
    }

    printf("Host: server port = %d\n", server_port);

    printf("Host: Creating enclave %s\n", enclave_path);
    enclave_ptr = create_enclave(enclave_path, flags);
    if (enclave_ptr == NULL)
    {
        goto exit;
    }

#ifdef __linux__
    // verify if SGX_AESM_ADDR is successfully set
    if (getenv("SGX_AESM_ADDR"))
    {
        printf("Host: environment variable SGX_AESM_ADDR is set\n");
    }
    else
    {
        printf("Host: environment variable SGX_AESM_ADDR is not set\n");
    }
#endif //__linux__
    res = generate_attestation(hw_mode, enclave_ptr);
    if (res != 0)
    {
        goto exit;
    }

    setup_card_mapping(enclave_ptr, &res);
    if (res != 0)
    {
        printf("Host: setup card mapping failed\n");
        goto exit;
    }

    printf("Host: calling run_server\n");
    run_server(enclave_ptr, &res, server_port);
    if (res != 0)
    {
        printf("Host: run_server failed\n");
        goto exit;
    }

    ret = 0;

exit:
    printf("Host: Terminating enclaves\n");
    if (enclave_ptr != NULL)
        terminate_enclave(enclave_ptr);

    printf("Host:  %s \n", (ret == 0) ? "succeeded" : "failed");
    return ret;

}

int verify_result(int argc, const char* argv[]) {
    const char * mode_option_sim = "-mode:sim";
    const char * mode_option_hw = "-mode:hw";
    const char * mrenclave_option = "-mrenclave:0x";

    int hw_mode, res;

    uint8_t expected_mrenclave[OE_UNIQUE_ID_SIZE] = {2};
    uint8_t signature[100];
    size_t signature_size;
    char games_won[100] = {0};
    size_t games_won_size;
    uint8_t public_key_data[200];
    size_t public_key_data_size;
    uint8_t report_data[5120];
    size_t report_data_size;

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    for (int j = 2; j < argc; j++) {
        if (strncmp(argv[j], mode_option_sim, strlen(mode_option_sim)) == 0) {
            printf("Host: Using simulation mode\n");
            hw_mode = 2;
        } else if (strncmp(argv[j], mode_option_hw, strlen(mode_option_sim)) == 0) {
            printf("Host: Using hardware mode\n");
            hw_mode = 1;
        } else if (strncmp(argv[j], mrenclave_option, strlen(mrenclave_option)) == 0) {
            if (strlen((char*)(argv[j] + strlen(mrenclave_option))) == 2*OE_UNIQUE_ID_SIZE) {
                for (int i = 0; i < OE_UNIQUE_ID_SIZE; i++) {
                    res = sscanf((char*)(argv[j] + strlen(mrenclave_option) + i*2), "%2hhx", &expected_mrenclave[i]);
                    if (res != 1) {
                        printf("-mrenclave is not a valid hex string at position %d\n", i);
                        return -1;
                    }                    
                }
            } else {
                printf("Host: expected %d byte hex string for -mrenclave option\n", OE_UNIQUE_ID_SIZE);
                return -1;
            }
        }
    }
    if (hw_mode == 0) {
        printf("specify -mode:<hw or sim>\n");
        return -1;
    }

    printf("Host: verifying output_data/gameswon.txt\n");
    read_file("output_data/report.data", "rb", report_data, sizeof(report_data), &report_data_size);
    read_file("output_data/public_key.pem", "r", public_key_data, sizeof(public_key_data), &public_key_data_size);
    read_file("output_data/gameswon.txt", "r", (uint8_t *) games_won, sizeof(games_won), &games_won_size);
    read_file("output_data/signature.data", "rb", signature, sizeof(signature), &signature_size);

    if (verify_attestation(hw_mode, report_data, report_data_size, public_key_data, public_key_data_size, expected_mrenclave, 1)!= 0) {
        return -1;
    }

    if(verify_signature(public_key_data, public_key_data_size, (const uint8_t *) games_won, games_won_size, signature, signature_size)!=0){
        printf("Host: could not verify signature on number of games won.\n");
        return -1;
    } else {
        printf("Host: verified \"%s\"\n", games_won);
        return 0;
    }
}

int main(int argc, const char* argv[])
{
    if (argc >= 2) {
        if (strcmp(argv[1], "run") == 0) {
            return new_game(argc, argv);
        } else if (strcmp(argv[1], "verify") == 0) {
            return verify_result(argc, argv);
        }
    } 
    print_usage(argv[0]);
    return 1;
}
