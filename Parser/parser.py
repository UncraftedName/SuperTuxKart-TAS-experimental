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

from client import Client_Socket

test_path = "./test/tasfile"

def strToBytes(s):
    return s.encode('utf-8') + b'\x00'

def intToTwoBytes(n):
    return struct.pack('h', n)

def intToFourBytes(n):
    return struct.pack('i', n)

def floatToBytes(f):
    return struct.pack('f', f)


def processTASFields(fields):
    """process each field and extract information returning
    it in dictionary form


    Keyword arguments:
    fields -- list containing all fields

    Return:
    Dictionary -- dictionary containing info on framebulk
    """
    fields_bit_array = b'00000000000'
    print("length 0: ", len(fields_bit_array))
    acc     = fields[0]
    misc    = fields[1]
    ang     = fields[2]
    ticks   = fields[3]

    # process acceleration fields
    if acc[0] == 'a':
        fields_bit_array += b'1'
    else:
        fields_bit_array += b'0'
    if acc[1] == 'b':
        fields_bit_array += b'1'
    else:
        fields_bit_array += b'0'

    # process mischelleneous fields
    if misc[0] == 'f':
        fields_bit_array += b'1'
    else:
        fields_bit_array += b'0'
    if misc[1] == 'n':
        fields_bit_array += b'1'
    else:
        fields_bit_array += b'0'
    if misc[2] == 's':
        fields_bit_array += b'1'
    else:
        fields_bit_array += b'0'
    print("length 1: ", len(fields_bit_array))
    # process ticks field
    fields_bit_array += intToTwoBytes(int(ticks))
    print("length 2: ", len(fields_bit_array))

    # process turning angle
    fields_bit_array += floatToBytes(float(ang))
    print("length 3: ", len(fields_bit_array))
    return fields_bit_array


def processTASHeader(header):
    """
    """
    header_bit_array = b''

    results = re.findall("map \"([A-z ]+)\"", header[0])
    if not results: # check if list is empty
        print("Warning: Value for map not found.")
        return
    bit_output = strToBytes(results[0])
    header_bit_array += bit_output


    results = re.findall("kart_name \"([A-z ]+)\"", header[1])
    if not results: # check if list is empty
        print("Warning: Value for kart_name not found.")
        return
    bit_output = strToBytes(results[0])
    header_bit_array += bit_output


    results = re.findall("num_ai_karts \"([\d]+)\"", header[2])
    if not results: # check if list is empty
        print("Warning: Value for num_ai_karts not found.")
        return
    bit_output = intToFourBytes(int(results[0]))
    if len(bit_output) > 4:
        print("warning: Value for num_ai_karts is invalid.")
    header_bit_array += bit_output


    results = re.findall("num_laps \"(-?[\d]+)\"", header[3])
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
        payload += processTASFields(fields)

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
