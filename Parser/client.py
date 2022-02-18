import socket
import struct

addr = ('127.0.0.1', 27015) # host and port number
msg_len_size = 4 # will be used when receiving to read size of message as int

# Used to create a client socket
# Client socket should be closed by the server
class Client_Socket:
    def __init__(self):
        # Member socket initialized for ipv4 and tcp
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Attempts to connect to the server
    # prints an error and exits if connection unsuccessful
    def start(self):
        try:
            self.client_socket.connect(addr)
        except:
            print('Error: Unable to connect to host, run the Injector and try again.')
            exit(1)
        print ('connection successful')

    # sends a message from the client to the server
    def send(self, msg : bytes) -> None:
        # prepend the length of message in front of msg
        # using little endian for consistency
        package = struct.pack('<i', len(msg)) + msg
        self.client_socket.send(package)
        
    # receives a message from the server
    def recv(self) -> bytes:
        # get the total message length
        # using little endian for consistency
        msg_len = struct.unpack('<i', self.client_socket.recv(msg_len_size))
        if len(msg) <= 0:
            print('Error: Invalid message received from server')
            exit(1)
        full_msg = bytes()
        while msg_len > 0:
            msg = self.client_socket.recv(1024)
            full_msg += msg
            msg_len -= len(msg)
        return full_msg
