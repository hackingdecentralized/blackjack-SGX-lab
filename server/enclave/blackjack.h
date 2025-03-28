
#include <string>
#include <vector>
#include <map>

const std::vector<char> SUITS = {'D', 'H', 'S', 'C'};
const std::map<char, std::string> SUIT_NAMES = {
    {'C', "Clubs"}, {'S', "Spades"}, {'H', "Hearts"}, {'D', "Diamonds"}
};
const std::vector<char> RANKS = {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'};
const std::map<char, std::string> RANKS_NAMES = {
    {'A', "Ace"}, {'2', "Two"}, {'3', "Three"}, {'4', "Four"}, {'5', "Five"},
    {'6', "Six"}, {'7', "Seven"}, {'8', "Eight"}, {'9', "Nine"}, {'T', "Ten"},
    {'J', "Jack"}, {'Q', "Queen"}, {'K', "King"}
};
const std::map<std::string, int> VALUES = {
    {"A", 1}, {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7},
    {"8", 8}, {"9", 9}, {"T", 10}, {"J", 10}, {"Q", 10}, {"K", 10}
};

extern std::map<char, std::map<char, int>> CARD_MAPPING;

class Card {
    public:
        Card(const char suit, const char rank);
        std::string str() const ;
        std::string get_suit() const;
        std::string get_rank() const;
        std::string serialize() const;
    private:
        char suit;
        char rank;
};
class Hand {
    public:
        Hand(const std::vector<Card>& cards = {}) : cards(cards) {}  
        std::string str() const;
        int get_value() const;
        void add_card(const Card& card);
        bool is_empty() const;
        std::string serialize(int num_hidden) const;
    private:
        std::vector<Card> cards;
};
class Deck {
    public:
        Deck();
        std::string str() const;
        Card deal_card();
        void shuffle();
        bool is_empty() const;
    private:
        std::vector<Card> cards;
};

class Game {
    public:
        Game() : game_done_(false), winner_("") {
        }    

        bool game_done();
        int deal();
        int hit(); 
        int stand(); 
        int calculate_winner();
        std::string serialize();
        std::string get_winner();
    private:
        Deck deck_;
        Hand player1_;
        Hand dealer_;
        bool game_done_;
        std::string winner_;
};