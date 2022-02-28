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
import argparse
from typing import List, Tuple, Callable

from client import Client_Socket, MessageType

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

# header keywords

KW_MAP        = 'map'
KW_KART_NAME  = 'kart_name'
KW_NUM_LAPS   = 'num_laps'
KW_DIFFICULTY = 'difficulty'
KW_NUM_AI     = 'num_ai_karts'

# This has some maps that you normally shouldn't be able to load,
# but I don't know/care enough to filter through all of these.
MAP_NAMES = {
    "abyss",
    "alien_signal",
    "ancient_colosseum_labyrinth",
    "arena_candela_city",
    "battleisland",
    "black_forest",
    "candela_city",
    "cave",
    "cocoa_temple",
    "cornfield_crossing",
    "endcutscene",
    "featunlocked",
    "fortmagma",
    "gplose",
    "gpwin",
    "gran_paradiso_island",
    "hacienda",
    "icy_soccer_field",
    "introcutscene",
    "introcutscene2",
    "lasdunasarena",
    "lasdunassoccer",
    "lighthouse",
    "mines",
    "minigolf",
    "olivermath",
    "overworld",
    "pumpkin_park",
    "ravenbridge_mansion",
    "sandtrack",
    "scotland",
    "snowmountain",
    "snowtuxpeak",
    "soccer_field",
    "stadium",
    "stk_enterprise",
    "temple",
    "tutorial",
    "volcano_island",
    "xr591",
    "zengarden",
}

KART_NAMES = {
    "adiumy",
    "amanda",
    "beastie",
    "emule",
    "gavroche",
    "gnu",
    "hexley",
    "kiki",
    "konqi",
    "nolok",
    "pidgin",
    "puffy",
    "sara_the_racer",
    "sara_the_wizard",
    "suzanne",
    "tux",
    "wilber",
    "xue",
}

# framebulk keywords

KW_FRAMES    = 'frames'
KW_PLAYSPEED = 'playspeed'


def define_field(key: str, pattern: str = r'[^\s\'"]+') -> str:
    # Returns a regex pattern with a named group for a key/value pair, where 'pattern'
    # is the regex for the value. The value may optionally be single or double quoted.
    # E.g. (key='map', pattern='\w+') will match "map 'e'" and groupdict()['map']='e'.
    return rf"""^{key}\s+=?\s*(?:"(?={pattern}")|'(?={pattern}'))?(?P<{key}>{pattern})['"]?$"""


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


def processTASHeader(lines: List[Tuple[int, str]]) -> bytes:
    """parse script header and convert to bytes

    Keyword arguments:
    lines -- all lines in the script before the 'frames' keyword,
        these lines are assumed to have no leading/trailing whitespace
    """

    header_fields_regex = '|'.join((
        define_field(KW_MAP),
        define_field(KW_KART_NAME),
        define_field(KW_NUM_LAPS),
        define_field(KW_DIFFICULTY),
        define_field(KW_NUM_AI),
    ))

    fields_dict = {}
    for m, line_num, line in ((re.match(header_fields_regex, line[1]), *line) for line in lines if line):
        if m:
            for k, v in m.groupdict().items():
                if v is not None:
                    fields_dict[k] = v
        else:
            print(f'Warning, unrecognized syntax on line {line_num}: "{line}", ignoring.')

    # validate header values

    # check that all mandatory keys exist
    for key in {KW_MAP, KW_KART_NAME, KW_NUM_LAPS, KW_DIFFICULTY} - fields_dict.keys():
        print(f"Value for '{key}' not found.")
        exit(1)

    def validate_field(key: str, convert_func: Callable, validate_func: Callable) -> bool:
        # key - header key
        # convert_func - tries to convert the value to a new value (e.g. str -> int)
        # validate_func - returns true if the new value is valid
        try:
            new_val = convert_func(fields_dict[key])
            success = validate_func(new_val)
            if success:
                fields_dict[key] = new_val
            return success
        except:
            return False

    if not validate_field(KW_NUM_LAPS, lambda s: int(s), lambda n: n == -1 or n > 0):
        print(f"Invalid value '{fields_dict[KW_NUM_LAPS]}' for key '{KW_NUM_LAPS}', \
            should be a positive integer or -1 in special cases.")
        exit(1)

    if not validate_field(KW_DIFFICULTY, lambda s: int(s), lambda n: n >= 0 and n <= 3):
        print(f"Invalid value '{fields_dict[KW_DIFFICULTY]}' for key '{KW_DIFFICULTY}', should be 0-3.")
        exit(1)

    if KW_NUM_AI not in fields_dict:
        fields_dict[KW_NUM_AI] = 0
        if not validate_field(KW_NUM_AI, lambda s: int(s), lambda n: n >= 0):
            print(f"Invalid value '{fields_dict[KW_NUM_AI]}' for key '{KW_NUM_AI}', should be at least 0.")
            exit(1)

    if fields_dict[KW_MAP] not in MAP_NAMES:
        print(
            f"Invalid map name: '{fields_dict[KW_MAP]}'. Here's a list of all valid maps:\n",
            '\n'.join(sorted(MAP_NAMES))
        )
        exit(1)

    if fields_dict[KW_KART_NAME] not in KART_NAMES:
        print(
            f"Invalid kart name: '{fields_dict[KW_KART_NAME]}'. Here's a list of all valid kart names:\n",
            '\n'.join(sorted(KART_NAMES))
        )
        exit(1)

    return (
        encodeStr(fields_dict[KW_MAP]) +
        encodeStr(fields_dict[KW_KART_NAME]) +
        struct.pack('3i', *(fields_dict[x] for x in (KW_NUM_AI, KW_NUM_LAPS, KW_DIFFICULTY)))
    )


def processTASLines(lines: List[Tuple[int, str]]):
    """process lines from TAS script and return information
    in dictionary format


    Keyword arguments:
    data -- list containing lines from TAS script

    Return:
    Dictionary -- dictionary containing all data in parsed TAS script
    """

    # find first line with 'frames' on it (gives index, not line number)
    try:
        framebulks_sep_line = next(i for i, line in enumerate(lines) if line[1].lower() == KW_FRAMES)
    except StopIteration:
        print(f"No '{KW_FRAMES}' keyword found, not sure where header ends.")
        exit(1)

    header = processTASHeader(lines[:framebulks_sep_line])

    # parse framebulks

    # syntax for framebulks:
    # --|---|-|0|
    # - indicate a field
    # 0 indicates number of ticks

    payload = b''
    for line_num, line in lines[framebulks_sep_line+1:]:
        if line.lower().startswith(KW_PLAYSPEED):
            field_bits = FLAG_SET_SPEED
            # get everything after 'playspeed' and interpret as float, send as an angle
            ang = float(line[len(KW_PLAYSPEED):])
            ticks = 0
        else:
            fields = [x for x in line.split("|") if x and not x.isspace()] # removes spaces from list elems
            if len(fields) != 4:
                print(f"Warning: Error parsing framebulk (line {line_num}). Exiting...")
                exit(1)
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
    # remove comments, strip whitespace, and remove blank lines
    # gives a int/string pair where the int is the line number
    lines: List[Tuple[int, str]] = [y for y in enumerate((removeComments(x).strip() for x in file.readlines()), start=1) if y[1]]

    file.close()

    return processTASLines(lines)

def getTASPath():
    """Parses command line arguments and retrieves a path. 
    If no path is given then the default path is used instead.

    Return:
    str -- String representing the path to the TAS script to be parsed
    """
    default = "./scripts/tasfile.peng"
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--path', type=str)
    args = parser.parse_args()
    if args.path == None:
        print("Notice: No path given. Using default path:", default)
        return default
    return args.path

def main():
    # Get path to TAS script
    tas_path = getTASPath()
    tasInfo = parseTAS(tas_path)

    # Open Client socket and send data to Payload
    sock = Client_Socket()
    sock.start()
    sock.send(tasInfo, MessageType.Script)


if __name__ == "__main__":
    main()
