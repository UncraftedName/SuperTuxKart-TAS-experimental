# ==================================
# Game actions:
#   Steer left/right
#   Accelerate back/forward
#   Fire
#   Nitro
#   Skidding
#   Look back
#   Rescue
# ==================================

import pathlib
import re
import struct
import ctypes

from client import Client_Socket

test_path = "./test/tasfile.peng"

def strToBytes(s) -> bytes:
    return s.encode('utf-8') + b'\x00'

def intToTwoBytes(n) -> bytes:
    return struct.pack('h', n)

def intToFourBytes(n) -> bytes:
    return struct.pack('i', n)

def floatToBytes(f) -> bytes:
    return struct.pack('f', f)


def fieldsToBytes(fields) -> bytes:
    acc     = fields[0]
    misc    = fields[1]
    ang     = fields[2]
    ticks   = fields[3]

    field_bits = 0

    # accel fields

    if acc[0] == 'a':
        field_bits |= 1
    if acc[1] == 'b':
        field_bits |= 1 << 1

    # mischelleneous fields

    if misc[0] == 'f':
        field_bits |= 1 << 2
    if misc[1] == 'n':
        field_bits |= 1 << 3
    if misc[2] == 's':
        field_bits |= 1 << 4
    
    # unused + flags -> 16 bits
    out = struct.pack('h', field_bits)
    # ticks field
    out = intToTwoBytes(int(ticks)) + out
    # turning angle
    out = floatToBytes(float(ang)) + out
    return out


def processTASHeader(header):
    """
    """
    header_bit_array = b''

    results = re.findall(r'map\s+"([A-z ]+)"', header[0])
    if not results: # check if list is empty
        print("Warning: Value for map not found.")
        return
    bit_output = strToBytes(results[0])
    header_bit_array += bit_output


    results = re.findall('kart_name\s+"([A-z ]+)"', header[1])
    if not results: # check if list is empty
        print("Warning: Value for kart_name not found.")
        return
    bit_output = strToBytes(results[0])
    header_bit_array += bit_output


    results = re.findall('num_ai_karts\s+([\d]+)', header[2])
    if not results: # check if list is empty
        print("Warning: Value for num_ai_karts not found.")
        return
    bit_output = intToFourBytes(int(results[0]))
    if len(bit_output) > 4:
        print("warning: Value for num_ai_karts is invalid.")
    header_bit_array += bit_output


    results = re.findall('num_laps\s+(-?[\d]+)', header[3])
    if not results: # check if list is empty
        print("Warning: Value for num_laps not found.")
        return
    bit_output = intToFourBytes(int(results[0]))
    if len(bit_output) > 4:
        print("warning: Value for num_laps is invalid.")
    header_bit_array += bit_output

    header_length = len(header_bit_array)
    # print("header_length: ", header_length)
    header_length_bits = intToFourBytes(header_length)
    # print("header_length bits: ", header_length_bits)
    if len(header_length_bits) > 4:
        print("warning: Header is too large. Exiting...")
        exit()

    return header_length_bits + header_bit_array


def processTASLines(data):
    """process lines from TAS script and return information
    in dictionary format


    Keyword arguments:
    data -- list containing lines from TAS script

    Return:
    Dictionary -- dictionary containing all data in parsed TAS script
    """
    header = processTASHeader(data[:4])
    # begin parsing framebulks
    if not "frames" in data[4]:
        print("Error: Did not find start of framebulks. Exiting...")
        exit(-1)
    # syntax for framebulks:
    # --|---|-|0|
    # - indicate a field
    # 0 indicates number of ticks
    payload = b''
    line_num = 3
    for line in data[5:]:
        line_num += 1
        print("line: \"" + str(line) + "\"")
        fields = [x for x in line.split("|") if x and not x.isspace()] # removes spaces from list elems
        print("fields: ", fields)
        if len(fields) != 4:
            print("Warning: Error parsing framebulk (line " + str(line_num) + "). Exiting...")
            exit()
        payload += fieldsToBytes(fields)

    # print("payload: ", payload)
    # print("length of payload: ", len(payload))
    payload_length_bits = intToFourBytes(len(payload))
    return header + payload_length_bits + payload

def removeComments(string): # Removes all comments from script
    return string[:string.find("//")]

def parseTAS(tasFile):
    """parse TAS file


    Keyword arguments:
    tasFile -- TAS filename
    """
    
    print("In parseTAS")
    path = pathlib.Path(tasFile)
    if not path.is_file():
        print("Error: File does not exist. Exiting...")
        exit(-1)

    file = open(tasFile, mode='r')

    data = [removeComments(x) for x in file.readlines() if (x != "\n")] # removes empty lines from list

    file.close()

    return processTASLines(data)


def main():
    """
    """
    global test_path
    tasInfo = parseTAS(test_path)
    print("final tas bitarray:", tasInfo)

    # Open Client socket and send data to Payload
    sock = Client_Socket()
    sock.start()
    sock.send(tasInfo)


if __name__ == "__main__":
    main()
