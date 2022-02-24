# =================================================
# Client Socket usage:
#   Create an object of Client_Socket: sock
#   Call sock.start() to connect to the IPC server
#
#   To send data, call sock.send(msg, type) where
#   'msg' is a bytes object and type indicates the
#   type of message sent.
#
#   To receive data, call sock.recv() which returns
#   a bytes object retrieved from the server.
# =================================================

import socket
import struct
from enum import Enum


class MessageType(Enum):
    # 0-255 range
    Script = 0 # we're sending a TAS script
    Unload = 1 # we're telling the payload to rid itself


addr = ("127.0.0.1", 27015) # IPC connection address


class Client_Socket:
    """Client_Socket class
    
    Used to create a socket that can send and receive tcp packets
    from a server

    Client_Socket should be closed by the server once IPC transfers
    are completed
    """

    def __init__(self):
        """initialize Client_Socket object
        
        Creates a socket that connects to an ipv4 address, and sends data
        using tcp
        """
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print("Successfully initialized socket")

    # Attempts to connect to the server
    # prints an error and exits if connection unsuccessful
    def start(self):
        """Attempts to connect to a server at addr

        Either prints a message declaring an error, or that connection was successful

        Keyword arguments:
        none

        Return:
        none
        """
        try:
            self.client_socket.connect(addr)
        except:
            print("Error: Unable to connect to host, run the Injector and try again.")
            exit(1)
        print ("connection successful")


    def send(self, msg : bytes, type: MessageType) -> None:
        """sends a message from the client to the server

        Keyword arguments:
        msg -- a bytes object which should be sent to the server
        type -- the message type

        Return:
        none
        """
        # Send the length of the full message as four bytes, then the message type,
        # then the full message, data is sent in host endian.
        msg = struct.pack('iB', len(msg) + 1, type.value) + msg
        self.client_socket.send(msg)
        

    def recv(self) -> bytes:
        """retrieves a message from the server

        Keyword arguments:
        none

        Return:
        bytes() -- a bytes object containing the message sent by the server
        """
        # stores the number of bytes in message into msg_len
        msg_len = struct.unpack("i", self.client_socket.recv(4))
        if len(msg) <= 0:
            print("Error: Invalid message received from server")
            exit(1)
        # loop until msg_len bytes are received from the server
        full_msg = bytes()
        while msg_len > 0:
            msg = self.client_socket.recv(1024)
            full_msg += msg
            msg_len -= len(msg)
        return full_msg
