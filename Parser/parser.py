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
from typing import List

from client import Client_Socket, MessageType

test_path = "./test/tasfile.peng"

def encodeStr(s):
    return s.encode('utf-8') + b'\x00'

def encodeShort(n):
    return struct.pack('h', n)

def encodeInt(n):
    return struct.pack('i', n)

def encodeFloat(f):
    return struct.pack('f', f)


FLAG_ACCEL     = 1
FLAG_BREAK     = 1 << 1
FLAG_ABILITY   = 1 << 2
FLAG_NITRO     = 1 << 3
FLAG_SKID      = 1 << 4
FLAG_SET_SPEED = 1 << 5

KEYWORD_PLAYSPEED = 'playspeed'


def encodeFramebulk(field_bits: int, turn_ang: float, num_ticks: int) -> bytes:
    return struct.pack('hhf', field_bits, num_ticks, turn_ang)


def getFieldBits(fields: List[str]) -> int:
    acc  = fields[0]
    misc = fields[1]

    field_bits = 0

    # accel fields

    if acc[0] == 'a':
        field_bits |= FLAG_ACCEL
    if acc[1] == 'b':
        field_bits |= FLAG_BREAK

    # mischelleneous fields

    if misc[0] == 'f':
        field_bits |= FLAG_ABILITY
    if misc[1] == 'n':
        field_bits |= FLAG_NITRO
    if misc[2] == 's':
        field_bits |= FLAG_SKID
    
    return field_bits


def processTASHeader(header):
    """
    """
    header_bit_array = b''

    results = re.findall(r'map\s+"([A-z ]+)"', header[0])
    if not results: # check if list is empty
        print("Warning: Value for map not found.")
        return
    bit_output = encodeStr(results[0])
    header_bit_array += bit_output


    results = re.findall('kart_name\s+"([A-z ]+)"', header[1])
    if not results: # check if list is empty
        print("Warning: Value for kart_name not found.")
        return
    bit_output = encodeStr(results[0])
    header_bit_array += bit_output


    results = re.findall('num_ai_karts\s+([\d]+)', header[2])
    if not results: # check if list is empty
        print("Warning: Value for num_ai_karts not found.")
        return
    bit_output = encodeInt(int(results[0]))
    header_bit_array += bit_output


    results = re.findall('num_laps\s+(-?[\d]+)', header[3])
    if not results: # check if list is empty
        print("Warning: Value for num_laps not found.")
        return
    bit_output = encodeInt(int(results[0]))
    header_bit_array += bit_output

    return header_bit_array


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
        if line.lower().startswith(KEYWORD_PLAYSPEED):
            field_bits = FLAG_SET_SPEED
            # get everything after 'playspeed' and interpret as float, send as an angle
            ang = float(line[len(KEYWORD_PLAYSPEED):])
            ticks = 0
        else:
            fields = [x for x in line.split("|") if x and not x.isspace()] # removes spaces from list elems
            if len(fields) != 4:
                print(f"Warning: Error parsing framebulk (line {line_num}). Exiting...")
                exit()
            field_bits = getFieldBits(fields)
            ang = float(fields[2])
            ticks = int(fields[3])
        payload += encodeFramebulk(field_bits, ang, ticks)

    return header + payload

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
    # strip whitespace, remove comments, etc.
    data = [y for y in (removeComments(x).strip() for x in file.readlines()) if len(y) != 0]

    file.close()

    return processTASLines(data)


def main():
    """
    """
    tasInfo = parseTAS(test_path)
    print("final tas bitarray:", tasInfo)

    # Open Client socket and send data to Payload
    sock = Client_Socket()
    sock.start()
    sock.send(tasInfo, MessageType.Script)


if __name__ == "__main__":
    main()
