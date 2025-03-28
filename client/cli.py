import client
HOST = "localhost"
PORT = 12341

def start_window_blackjack():
    print("Dealing cards...")
    update_window_blackjack(client.deal(HOST, PORT)) 

def update_window_blackjack(res):
    player_cards = res["player_cards"]
    dealer_cards = res["dealer_cards"]

    print("Dealer:", [c[0] for c in dealer_cards])
    print("You:", [c[0] for c in player_cards])
    if "result" in res:
        final_window_blackjack(res)
        return

    res = input("Hit (H/h) or Stand (S/s): ")
    while res not in ['H', 'h', 'S', 's']:
        print(res, "is invalid")
        res = input("Hit (H/h) or Stand (S/s): ")
    
    if res in ['H', 'h']:
        print("You choose to hit!")
        update_window_blackjack(client.hit(HOST, PORT))

    if res in ['S', 's']:
        print("You choose to stand!")
        update_window_blackjack(client.stand(HOST, PORT))

def final_window_blackjack(res):
    player_score, dealer_score = res["result"] 
    message = "You Lost!"
    if (dealer_score < player_score or dealer_score > 21) and player_score <= 21:
        message = "You Won!"

    print(f'{message}\n[You: {player_score}]\n[Dealer: {dealer_score}]\n')

    res = input("Deal (D/d) or Exit (E/e): ")
    while res not in ['D', 'd', 'E', 'e']:
        print(res, "is invalid")
        res = input("Deal (D/d) or Exit (E/e): ")
    
    if res in ['D', 'd']:
        print("You choose to deal!")
        client.start(HOST, PORT)
        update_window_blackjack(client.deal(HOST, PORT))

    if res in ['E', 'e']:
        print("You choose to exit!")
        client.exit(HOST, PORT)


def load_userid_port(filename="../netid_ports.txt"):
    import os, sys
    user = os.getenv("USER")
    if not user:
        sys.exit("Error: USER environment variable not set")

    try:
        with open(filename) as f:
            for line in f:
                if line.startswith(f"{user}:"):
                    return user, int(line.strip().split(":")[1])
    except FileNotFoundError:
        sys.exit(f"Error: {filename} not found")

    sys.exit(f"Error: No port found for user '{user}' in {filename}")

if __name__ == '__main__':
    from sys import argv
    if len(argv) == 1:
        HOST = "localhost"
        userid, PORT = load_userid_port()

    elif len(argv) == 4:
        userid = argv[1]
        HOST = argv[2]
        PORT = int(argv[3])
    else:
        sys.exit(f"Usage: python3 [userid] [host] [port]")

    print(f"connecting to {HOST}:{PORT}...")
    client.start(HOST, PORT, userid)
    start_window_blackjack()