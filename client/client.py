
import json
import socket

SOCKET = None

def format_msg(msg):
    msg += b'\0'
    msg_len = len(msg)
    buff = msg_len.to_bytes(8, "little") + msg
    return buff

def decode_msg(s):
    res = b''
    while True:
        try:
            chunk = s.recv(4096) 
            if not chunk:
                break
            res += chunk
        except:
            break
    # msg_len = int.from_bytes(res[:8], byteorder='little') + 8
    # print(msg_len,len(res),res[8:100])
    return res[8:].decode('utf-8')

def start(host, port, userid=''):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.send(format_msg(b'N' + bytes(userid, "utf-8")))
    data = json.loads(decode_msg(s))
    s.close()

    assert data["status"], data["error"]

def deal(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.send(format_msg(b'D'))
    data = json.loads(decode_msg(s))
    s.close()
    if not data["status"]:
        print(data["error"])
    return data

def stand(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.send(format_msg(b'S'))
    data = json.loads(decode_msg(s))
    s.close()
    if not data["status"]:
        print(data["error"])
        exit(0)
    return data


def hit(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.send(format_msg(b'H'))
    data = json.loads(decode_msg(s))
    s.close()
    if not data["status"]:
        print(data["error"])
        exit(0)
    return data

def exit(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.send(format_msg(b'E'))
    s.close()

if __name__ == '__main__':
    start("localhost", 12341)
