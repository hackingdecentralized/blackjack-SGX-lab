import PySimpleGUI as sg
import client
import math
from PIL import Image
import io

HOST = None
PORT = 0
DARK_GREEN = "#06402B"
WHITE="#FFFFFF"
NUM_CARDS = 52
CARD_BACK_IMAGE = "../images/card_back.png"


def start_window_blackjack():
    sg.set_options(background_color=DARK_GREEN)
    NUM_PLAYERS = 2
    MAX_CARDS_PER_PLAYER = math.ceil(NUM_CARDS/NUM_PLAYERS)
    layout = [[sg.Text("Dealer", size=(6,1), text_color=WHITE, auto_size_text=True, font=" 16")],
              [sg.Col([[sg.Image(filename=CARD_BACK_IMAGE)] +[sg.Image(filename=CARD_BACK_IMAGE, visible=False, k=f"D{i}") for i in range(MAX_CARDS_PER_PLAYER)]])],
              [sg.Text("You", size=(3,1), text_color=WHITE, font=" 16")],
              [sg.Col([[sg.Image(filename=CARD_BACK_IMAGE)] +[sg.Image(filename=CARD_BACK_IMAGE, visible=False, k=f"P{i}") for i in range(MAX_CARDS_PER_PLAYER)]])],
              [sg.Col([[sg.Button('Deal', k='-Deal-'), sg.Button('Restart', k="-Restart-", visible=False), sg.Button('Exit', k="-Exit-", visible=False), sg.Button('Hit', k="-Hit-", visible=False), sg.Button('Stand', k="-Stand-", visible=False)]], k="-Buttons-")],
              [sg.Text("You ????\n[You 00]\n[Dealer 00]\n\n", text_color=WHITE, size=(20,5), k="-Result-", visible=False, justification="center", font=" 32")]]
    window = sg.Window('Blackjack', layout)
    event, _ = window.read()
    if event == sg.WIN_CLOSED:
        window.close()
        exit(0)
    elif event == "-Deal-":
        update_window_blackjack(window, client.deal(HOST, PORT), change_buttons=True)
    else:
        window.close()
        exit(0)

def convert_bmp_to_png(bmp_data):
    from PIL import ImageFile
    ImageFile.LOAD_TRUNCATED_IMAGES = True
    bmp_image = Image.open(io.BytesIO(bmp_data))
    png_buffer = io.BytesIO()
    bmp_image.save(png_buffer, format="PNG")
    png_data = png_buffer.getvalue()
    return png_data

def update_window_blackjack(window, res, change_buttons=False):

    player_cards = res["player_cards"]
    dealer_cards = res["dealer_cards"]
    for i in range(len(player_cards)):
        window[f"P{i}"].update(visible=True, data=convert_bmp_to_png(bytes.fromhex(player_cards[i][1])))
    for i in range(len(dealer_cards)):
        if dealer_cards[i][0] != "Hidden":
            window[f"D{i}"].update(visible=True, data=convert_bmp_to_png(bytes.fromhex(dealer_cards[i][1])))
        else:
            window[f"D{i}"].update(visible=True, filename=CARD_BACK_IMAGE)

    if change_buttons:
        window[('-Deal-')].update(visible=False)
        window[('-Hit-')].update(visible=True)
        window[('-Stand-')].update(visible=True)

    if "result" in res:
        final_window_blackjack(window, res)

    event, _ = window.read()
    if event == sg.WIN_CLOSED:
        window.close()
        exit(0)
    elif event == "-Hit-":
        update_window_blackjack(window, client.hit(HOST, PORT))
    elif event == "-Stand-":
        update_window_blackjack(window, client.stand(HOST, PORT))
    else:
        window.close()
        exit(0)    

def final_window_blackjack(window, res):
    window[('-Hit-')].update(visible=False)
    window[('-Stand-')].update(visible=False)
    window[('-Restart-')].update(visible=True)
    window[('-Exit-')].update(visible=True)

    player_score, dealer_score = res["result"] 
    message = "You Lost!"
    if (dealer_score < player_score or dealer_score > 21) and player_score <= 21:
        message = "You Won!"
    window["-Result-"].update(visible=True, value=f'{message}\n[You: {player_score}]\n[Dealer: {dealer_score}]\n\n')

    event, _ = window.read()
    if event == sg.WIN_CLOSED:
        client.exit(HOST, PORT)
        window.close()
        exit(0)
    elif event == "-Exit":
        window.close()
        client.exit(HOST, PORT)
    elif event == "-Restart-":
        window.close()
        client.start(HOST, PORT)
        start_window_blackjack()
    else:
        client.exit(HOST, PORT)
        window.close()
        exit(0)


if __name__ == '__main__':
    from sys import argv
    if len(argv) < 2:
        print("enter userid as first argument")
        exit(1)
    userid = argv[1]
    if len(argv) > 2:
        HOST = argv[2]
        PORT = int(argv[3])
    else:
        HOST = "localhost"
        PORT = 12341
    print(f"connecting to {HOST}:{PORT}...")
    client.start(HOST, PORT, userid)
    start_window_blackjack()