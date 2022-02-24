# =================================================
# Connects to the payload and tells it to unload.
# =================================================


from client import Client_Socket, MessageType


def main():
    sock = Client_Socket()
    sock.start()
    sock.send(b'', MessageType.Unload)
    print('DLL successfully unloaded')


if __name__ == '__main__':
    main()
