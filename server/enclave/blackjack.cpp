#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <openenclave/corelibc/stdlib.h>

#include "blackjack.h"
#include "blackjack_t.h"
#include "log.h"
#include "sealing.h"

int NUM_HIDDEN_CARDS = 1;
int SMART_DEALER = 1;

// Card class
Card::Card(const char suit, const char rank) {
    if (std::find(SUITS.begin(), SUITS.end(), suit) != SUITS.end() &&
        std::find(RANKS.begin(), RANKS.end(), rank) != RANKS.end()) {
        this->suit = suit;
        this->rank = rank;
    }
}    
std::string Card::str() const {
    std::string res = RANKS_NAMES.at(rank);
    res += " of ";
    res += SUIT_NAMES.at(suit);
    return res;
}    
std::string Card::get_suit() const {
    return std::string(1, suit);
}    
std::string Card::get_rank() const {
    return std::string(1, rank);
}    
std::string Card::serialize() const {
    uint8_t seal_card_data[SEAL_CARD_SIZE] = {0};
    size_t card_data_size;
    char hex_buffer[CARD_SIZE*2] = {0};
    int res;
    const char card_name[2] = {rank, suit};
    int mapping = CARD_MAPPING[rank][suit];
    char path[24] = {0};
    uint8_t * card_data;

    sprintf(path, "%02d.seal",mapping);
    
    read_card(&res, path, seal_card_data, sizeof(seal_card_data));
    if (res != 0) {
          TRACE_ENCLAVE("failed to read card\n");
        return "";
    }


    res = unseal_data((const uint8_t*) seal_card_data, sizeof(seal_card_data), (const uint8_t *) card_name,
                            sizeof(card_name), &card_data, &card_data_size);

    if (res != 0) {
          TRACE_ENCLAVE("failed to unseal card\n");
        return "";
    }

    if (card_data_size != CARD_SIZE) {
          TRACE_ENCLAVE("unexpected card size %lu!=%d\n", card_data_size, CARD_SIZE);
        return "";
    }

    for (size_t i = 0; i < card_data_size; i++) {
        sprintf(hex_buffer+(i*2), "%02x", (unsigned char) card_data[i]);
    }
    std::string hex_data(hex_buffer, card_data_size*2);
    // oe_free(card_data);

    return "[\"" + str() + "\",\"" + hex_data + "\"]";
}

std::string Hand::str() const {
    std::string result;
    for (const auto& card : cards) {
        result += card.str() + " ";
    }
    return result;
}    
void Hand::add_card(const Card& card) {
    cards.push_back(card);
}    
int Hand::get_value() const {
    int value = 0;
    for (const auto& card : cards) {
        value += VALUES.at(card.get_rank());
    }
    for (const auto& card : cards) {
        if (card.get_rank() == "A" && value <= 11) {
            value += 10;
        }
    }
    return value;
}    
bool Hand::is_empty() const {
    return cards.empty();
}    

std::string Hand::serialize(int num_hidden) const {
    std::vector<std::string> result;
    for (const auto& card : cards) {
        result.push_back(card.serialize());
    }
    std::string s = "[";
    for (size_t i = 0; i < result.size(); ++i) {
        if (i < num_hidden) {
            s += "[\"Hidden\",\"Hidden\"]";
        } else {
            s +=  result[i];
        }
        if (i != result.size()-1) {
            s += ",";
        }
    }
    s += "]";
    return s;
}

Deck::Deck() {
    for (const auto& suit : SUITS) {
        for (const auto& rank : RANKS) {
            cards.emplace_back(suit, rank);
        }
    }
    shuffle();
}    
std::string Deck::str() const {
    std::string result;
    for (const auto& card : cards) {
        result += card.str() + " ";
    }
    return result;
}    
void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(cards.begin(), cards.end(), gen);
}    

Card Deck::deal_card() {
    Card card = cards.back();
    cards.pop_back();
    // TRACE_ENCLAVE("deal %s\n", card.str().c_str());
    return card;
}    
bool Deck::is_empty() const {
    return cards.empty();
}

bool Game::game_done() {
    return game_done_;
}

int Game::deal() {
    if (game_done_) {
        TRACE_ENCLAVE("deal game done!\n");
        return -1;
    }
    for (int i = 0; i < 2; ++i) {
        player1_.add_card(deck_.deal_card());
        dealer_.add_card(deck_.deal_card());
    }
    return 0;
}    

int Game::hit() {
    if (game_done_) {
          TRACE_ENCLAVE("hit game done!\n");
        return -1;
    }
    player1_.add_card(deck_.deal_card());
    int player_score = player1_.get_value();

    if (SMART_DEALER == 1) {
        if (dealer_.get_value() < player_score &&  player_score <= 21) {
            TRACE_ENCLAVE("dealer hit\n");
            dealer_.add_card(deck_.deal_card());
        }
    } else {
        if (dealer_.get_value() < 17) {
            TRACE_ENCLAVE("dealer hit\n");
            dealer_.add_card(deck_.deal_card());
        }        
    }
    TRACE_ENCLAVE("dealer %d player %d\n", player_score, dealer_.get_value());
    return 0;

}    

int Game::stand() {
    if (game_done_) {
          TRACE_ENCLAVE("stand game done!\n");
        return -1;
    }
    game_done_ = true;
    int to_beat;
    if (SMART_DEALER == 1){
        to_beat = std::min(player1_.get_value(), 21);
    } else {
        to_beat = 17;
    }
    while (dealer_.get_value() < to_beat) {
        dealer_.add_card(deck_.deal_card());
    }
    return calculate_winner();

}    

int Game::calculate_winner() {
    if (!game_done_) {
          TRACE_ENCLAVE("calculate_winner game not done!\n");
        return -1;
    }
    if ((player1_.get_value() > dealer_.get_value() || dealer_.get_value() > 21) && player1_.get_value() <= 21) {
        winner_ = "player";
    } else {
        winner_ = "dealer";
    }
    return 0;

}    

std::string Game::get_winner() {
    return winner_;
}

std::string Game::serialize() {
    std::string output;
    auto player_score = player1_.get_value();
    auto dealer_score = dealer_.get_value();

    if (player_score > 21 || dealer_score > 21 || game_done_) {
        game_done_ = true;
        calculate_winner();
        auto player_cards_serialized = player1_.serialize(0);
        auto dealer_cards_serialized = dealer_.serialize(0);
        auto format = "{\"status\":true,\"player_cards\":%s,\"dealer_cards\":%s,\"result\":[%d, %d],\"winner\":\"%s\"}";
        auto size = std::snprintf(nullptr, 0, format, player_cards_serialized.c_str(), dealer_cards_serialized.c_str(), player_score, dealer_score, winner_.c_str());
        output = std::string(size + 1, '\0');
        std::sprintf(&output[0], format, player_cards_serialized.c_str(), dealer_cards_serialized.c_str(), player_score, dealer_score, winner_.c_str());
    } else {
        auto player_cards_serialized = player1_.serialize(0);
        auto dealer_cards_serialized = dealer_.serialize(NUM_HIDDEN_CARDS);
        auto format = "{\"status\":true,\"player_cards\":%s,\"dealer_cards\":%s}";
        auto size = std::snprintf(nullptr, 0, format, player_cards_serialized.c_str(), dealer_cards_serialized.c_str());
        output = std::string(size + 1, '\0');
        std::sprintf(&output[0], format, player_cards_serialized.c_str(), dealer_cards_serialized.c_str());
    }
    return output;
}
