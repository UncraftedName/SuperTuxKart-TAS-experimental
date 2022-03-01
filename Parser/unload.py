# =================================================
# Connects to the payload and tells it to unload.
# =================================================


from client import ClientSocket, MessageType


def main():
    sock = ClientSocket()
    sock.start()
    sock.send(b'', MessageType.Unload)
    print('DLL successfully unloaded')


if __name__ == '__main__':
    main()
