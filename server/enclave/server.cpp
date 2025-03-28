#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>
#include <unistd.h>

#include <openenclave/corelibc/stdlib.h>

#include "blackjack.h"
#include "blackjack_t.h"
#include "log.h"
#include "sealing.h"
#include "server.h"

extern "C"
{
    int set_up_server(int server_port);
};

const char NEW_GAME = 'N';
const char DEAL = 'D';
const char HIT = 'H';
const char STAND = 'S';
const char EXIT = 'E';

std::unique_ptr<Game> CURRENT_GAME_BLACKJACK = nullptr;
std::map<char, std::map<char, int>> CARD_MAPPING;
std::string userid;

const int MAX_GAMES = 10;
int games_won;
int total_games;

int process_request(char *request, uint64_t request_len, char ** response, uint64_t * response_len) {
    TRACE_ENCLAVE("process_request: %s\n", request);
    const char * response_str;
    std::string output_data;
    size_t response_str_len;

    if (request[0] == NEW_GAME) {
        if (CURRENT_GAME_BLACKJACK && !CURRENT_GAME_BLACKJACK->game_done()) {
            TRACE_ENCLAVE("active game exists\n");
            response_str = "{\"error\": \"active game exists\", \"status\": false}";
            response_str_len = strlen(response_str)+1;
        } else {
            TRACE_ENCLAVE("creating game\n");
            CURRENT_GAME_BLACKJACK = std::unique_ptr<Game>(new Game());
            if (request_len > 2) {
                userid = std::string(request + 1);
                TRACE_ENCLAVE("userid %s\n", userid.c_str());
            }
            response_str = "{\"status\": true}";
            response_str_len = strlen(response_str)+1;
        }
    } else if (request_len != 2) {
            response_str = "{\"error\": \"request incorrectly formatted\", \"status\": false}";
            response_str_len = strlen(response_str);
        } else { if (request[0] == DEAL) {
            if (!CURRENT_GAME_BLACKJACK || CURRENT_GAME_BLACKJACK->game_done()) {
                TRACE_ENCLAVE("no active games\n");
                response_str = "{\"error\": \"no active games\", \"status\": false}";
                response_str_len = strlen(response_str)+1;
            } else {
                TRACE_ENCLAVE("dealing cards\n");
                if(CURRENT_GAME_BLACKJACK->deal() == 0) {
                    TRACE_ENCLAVE("serializing game\n");
                    output_data = CURRENT_GAME_BLACKJACK->serialize();
                    response_str = output_data.c_str();
                    response_str_len = output_data.size();
                    if (CURRENT_GAME_BLACKJACK->game_done()) {
                        total_games++;
                        if (CURRENT_GAME_BLACKJACK->get_winner() == "player") {
                            games_won++;
                        }
                    }
                } else {
                    response_str = "{\"status\": false, \"error\": \"game done\"}";
                    response_str_len = strlen(response_str)+1;
                }
            }

        } else if (request[0] == HIT) {
            if (!CURRENT_GAME_BLACKJACK || CURRENT_GAME_BLACKJACK->game_done()) {
                TRACE_ENCLAVE("no active games\n");
                response_str = "{\"error\": \"no active games\", \"status\": false}";
                response_str_len = strlen(response_str)+1;
            } else {
                TRACE_ENCLAVE("hit\n");
                if(CURRENT_GAME_BLACKJACK->hit() == 0){
                    TRACE_ENCLAVE("serializing game\n");
                    output_data = CURRENT_GAME_BLACKJACK->serialize();
                    response_str = output_data.c_str();
                    response_str_len = output_data.size();
                    if (CURRENT_GAME_BLACKJACK->game_done()) {
                        total_games++;
                        if (CURRENT_GAME_BLACKJACK->get_winner() == "player") {
                            games_won++;
                        }
                    }
                } else {
                    response_str = "{\"status\": false, \"error\": \"game done\"}";
                    response_str_len = strlen(response_str)+1;
                }
            }
            // res.set_content(output_data, "application/json");
            // if (CURRENT_GAME_BLACKJACK->game_done()) {
            //     GAME_SCORES_BLACKJACK = Scores::update_scores(GAME_SCORES_BLACKJACK, output_data["result"]);
            // }
        } else if (request[0] == STAND) {
            if (!CURRENT_GAME_BLACKJACK || CURRENT_GAME_BLACKJACK->game_done()) {
                TRACE_ENCLAVE("no active games\n");
                response_str = "{\"error\": \"no active games\", \"status\": false}";
                response_str_len = strlen(response_str)+1;
            } else {
                TRACE_ENCLAVE("stand\n");
                if(CURRENT_GAME_BLACKJACK->stand() == 0){
                    TRACE_ENCLAVE("serializing game\n");
                    output_data = CURRENT_GAME_BLACKJACK->serialize();
                    response_str = output_data.c_str();
                    response_str_len = output_data.size();
                    if (CURRENT_GAME_BLACKJACK->game_done()) {
                        total_games++;
                        if (CURRENT_GAME_BLACKJACK->get_winner() == "player") {
                            games_won++;
                        }
                    }
                } else {
                    response_str = "{\"status\": false, \"error\": \"game done\"}";
                    response_str_len = strlen(response_str)+1;
                }
            }
            // res.set_content(output_data, "application/json");
            // if (CURRENT_GAME_BLACKJACK->game_done()) {
            //     GAME_SCORES_BLACKJACK = Scores::update_scores(GAME_SCORES_BLACKJACK, output_data["result"]);
            // }
        } else if (request[0] == EXIT) {
            return 1;
        } else {
            TRACE_ENCLAVE("request format error\n");
            response_str = "{\"error\": \"request incorrectly formatted\", \"status\": false}";
            response_str_len = strlen(response_str)+1;
        }
    }
    *response = (char *) oe_malloc(strlen(response_str)+1);
    if (*response == NULL) {
        TRACE_ENCLAVE("oe_malloc() out of memory\n");
        return -1;
    }
    memcpy(*response, response_str, strlen(response_str)+1);
    *response_len = strlen(response_str);
    return 0;
}

int handle_communication_until_done(int server_fd, struct sockaddr* address, socklen_t* addrlen) {
    int ret;
    int socket;
    char in_buffer[100] = {0};

    char * out_buffer;
    uint64_t out_buffer_size;
    uint64_t send_size;

    while(total_games <= MAX_GAMES) {
        if ((socket = accept(server_fd, address, addrlen)) < 0) {
            perror("accept connection failed");
            return -1;
        }
        read(socket, in_buffer, 100);
        if (total_games == MAX_GAMES) {
            const char * done_msg = "{\"status\":false, \"error\": \"max games played\"}";
            uint64_t done_msg_size = (uint64_t)strlen(done_msg);
            send(socket, (char *)&done_msg_size, sizeof(done_msg_size), 0);
            send(socket, done_msg, done_msg_size, 0);
        } else{
            ret = process_request((char *)(in_buffer + sizeof(uint64_t)), *((uint64_t *) in_buffer), &out_buffer, &out_buffer_size);
            if (ret == -1) {
                return -1;
            } else if (ret == 1) {
                close(socket);
                return games_won;
            }
            send_size = out_buffer_size+sizeof(uint64_t);
            send(socket, (char *) &send_size, sizeof(send_size), 0);
            send(socket, out_buffer, out_buffer_size, 0);
            if (out_buffer != NULL){
                oe_free(out_buffer);
                out_buffer = NULL;
            }
        }
        close(socket);
    }

    return games_won;
}


int create_listener_socket(int port, int& server_socket) {
    int ret = -1;
    const int reuse = 1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        goto exit;
    }

    if (setsockopt(server_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (const void*)&reuse,
            sizeof(reuse)) < 0) {
        perror("setsockopt failed");
        goto exit;
    }

    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind socket to port failed");
        goto exit;
    }

    if (listen(server_socket, 20) < 0)
    {
        perror("listen on socket failed");
        goto exit;
    }
    ret = 0;
exit:
    return ret;
}



