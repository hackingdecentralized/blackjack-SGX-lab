* The best way is to run the GUI client on your laptop, and connect to the server remotely
* python-tk is required by PySimpleGUI. Python 3.13 is recommended. Both can be installed by:
```
# on macOS
brew install python-tk@3.13
```

To verify python-tk is properly set up, run the following; there should not be any error (no output).

```
python3.13 -c "import tkinter"
```

* Create a virtual env and install PySimpleGUI using pip

```
python3.13 -m venv env
source env/bin/activate
pip3 install --upgrade --extra-index-url https://PySimpleGUI.net/install PySimpleGUI
```

* Install pillow `pip3 install pillow`

* with all dependencies installed, the GUI client can be started by

```
python3.13 gui.py netid server-ip your-assigned-port
```
* On startup you can follow the prompts to get a free trail/license.
