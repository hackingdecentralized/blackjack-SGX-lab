# Blackjack Enclave Challenge

_Acknowledgement: This lab is developed in collaboration with Prof. Andrew Miller and Nerla Jean-Louis at UIUC. The vast majority of the credit goes to them._

This repo contains a faulty or poorly designed blackjack game with a fatal flaw. 
You should be able to up your chances of beating our sophisticated AI dealer by modifying non-enclave files (i.e. not anything in [server/enclave](server/enclave/)).

In order to prove to another party that you successfully beat the challenge you should provide the generated by the server in [server/output_data](server/output_data). These files should be (gameswon.txt, public_key.pem, report.data, signature.data)

## Prerequesits

### Server

No additional setup required. All dependecies are provided.

<!-- We will mainly use simulation mode since it's easier to set up.  -->
<!-- Simulation means that SGX instructions are emulated by system software, commonly used for development and debugging. You can consult the full README for instructions to run everything in hardware mode.  -->

### Client

The main client you will modify to finish the task is the CLI client (`cli.py`), but we also provide a GUI client (`gui.py`) for your entertainment.

- No additional setup required for the CLI client.
- To run the GUI client, refer to gui-README.md.

## Build and run server

```bash
cd server
make build
make runsim
```

## Python client

Connect to the blackjack server using either the CLI interface
```bash
source venv/bin/activate
python3 cli.py
```

or the GUI interface:

```bash
source venv/bin/activate
python3 gui.py
```

## Verifying game results
Assuming you have gracefully exited the server processes. Once you have finished playing you can verify the results of the game.

```bash
cd server
make verifysim
```
