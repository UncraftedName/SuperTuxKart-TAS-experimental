# ==================================
# Game actions:
#   Steer left/right
#   Accelerate back/forward
#   Fire
#   Nitro
#   Skidding
#   Look back
# ==================================

import pathlib
import re
import struct
import argparse
import os
from typing import List, Tuple, Callable

from client import ClientSocket, MessageType

class Framebulk:

    class Flags:
        FLAG_ACCEL     = 1
        FLAG_BREAK     = 1 << 1
        FLAG_ABILITY   = 1 << 2
        FLAG_NITRO     = 1 << 3
        FLAG_SKID      = 1 << 4
        FLAG_SET_SPEED = 1 << 5

        def __init__(self, accel=False, decel=False, ability=False, nitro=False, skid=False, set_speed=False):
            self.accel = accel
            self.decel = decel
            self.ability = ability
            self.nitro = nitro
            self.skid = skid
            self.set_speed = set_speed

        @classmethod
        def from_script(cls, fields: List[str]):
            """Converts the framebulk fields to an int that has its bits set
            to denote what buttons should be pressed for this framebulk.

            Keyword arguments:
            fields -- each 'section' of the framebulk; <section 1>|<section 2>|...|
            """
            acc  = fields[0]
            misc = fields[1]
            return cls(acc[0] == 'a', acc[1] == 'b', misc[0] == 'f', misc[1] == 'n', misc[2] == 's')

        def to_int(self) -> int:
            """
            """
            field_bits = 0

            # accel fields

            if self.accel:
                field_bits |= self.FLAG_ACCEL
            if self.decel:
                field_bits |= self.FLAG_BREAK

            # miscellaneous fields

            if self.ability:
                field_bits |= self.FLAG_ABILITY
            if self.nitro:
                field_bits |= self.FLAG_NITRO
            if self.skid:
                field_bits |= self.FLAG_SKID
            if self.set_speed:
                field_bits |= self.FLAG_SET_SPEED

            return field_bits

        def __eq__(self, __o: object) -> bool:
            if type(__o) != Framebulk.Flags:
                return False
            return self.__dict__ == __o.__dict__

        def __repr__(self) -> str:
            return repr(self.__dict__)

    def __init__(self, num_ticks: int, angle: float, flags: Flags):
        self.num_ticks = num_ticks
        self.angle = angle
        self.flags = flags

    @classmethod
    def from_script(cls, line: str, line_num: int):
        """Converts framebulk into bytes

        Keyword arguments:
        str -- the line for the framebulk which this method is converting to bytes
        int -- the line number for the specific line
        """
        # https://stackoverflow.com/questions/12643009/regular-expression-for-floating-point-numbers
        playspeed_re = define_field(
            KW_PLAYSPEED,
            r'[+-]?(?:\d+(?:[.]\d*)?(?:[eE][+-]?\d+)?|[.]\d+(?:[eE][+-]?\d+)?)'
        )

        m = re.match(playspeed_re, line)
        if m:
            # special playspeed framebulk - 0 ticks, angle is treated as new play speed
            return cls(0, float(m.groupdict()[KW_PLAYSPEED]), Framebulk.Flags(set_speed=True))
        else:
            # syntax for framebulks:
            # --|---|-|ticks|
            # '-' indicates a button/field
            fields = [x for x in line.split("|") if x and not x.isspace()]  # removes spaces from list elems
            if len(fields) != 4:
                print(f"Warning: Error parsing framebulk (line {line_num}). Exiting...")
                exit(1)
            return cls(int(fields[3]), float(fields[2]), Framebulk.Flags.from_script(fields))

    def encode(self) -> bytes:
        """Turns itself into bytes using the flags attribute as 
        well as number of ticks and angle
        """
        return struct.pack('hhf', self.flags.to_int(), self.num_ticks, self.angle)

    def __eq__(self, __o: object) -> bool:
        if type(__o) != Framebulk:
            return False
        return self.__dict__ == __o.__dict__

    def __repr__(self) -> str:
        return repr(self.__dict__)

# header keywords
KW_MAP         = 'map'
KW_KART_NAME   = 'kart_name'
KW_NUM_LAPS    = 'num_laps'
KW_DIFFICULTY  = 'difficulty'
KW_NUM_AI      = 'num_ai_karts'
KW_QUICK_RESET = 'quick_reset'

# maps that can be loaded
MAP_NAMES = {
    "abyss",
    "black_forest",
    "candela_city",
    "cocoa_temple",
    "cornfield_crossing",
    "fortmagma",
    "gran_paradiso_island",
    "hacienda",
    "lighthouse",
    "mines",
    "minigolf",
    "olivermath",
    "overworld", # can be loaded, but not an actual race track
    "ravenbridge_mansion",
    "sandtrack",
    "scotland",
    "snowmountain",
    "snowtuxpeak",
    "stk_enterprise",
    "tutorial",
    "volcano_island",
    "xr591",
    "zengarden",
}

# characters that can race
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
KW_HEADER_END = 'framebulks'
KW_PLAYSPEED  = 'playspeed'


def define_field(key: str, pattern: str = r'[^\s\'"]+') -> str:
    """Gives a regex pattern for a key/value with a named group for value; 'pattern'
    is the regex for the value. The value may optionally be single or double quoted.
    E.g. key='map', pattern='\w+' will match "map 'e'" and groupdict()['map']='e'
    """
    return rf"""^{key}(?:\s+|\s*[:=]\s*)(?:"(?={pattern}")|'(?={pattern}'))?(?P<{key}>{pattern})['"]?$"""


def encode_header(fields_dict: dict) -> bytes:
    """Converts a dictionary representing fields into bytes

    Keyword arguments:
    dictionary -- dictionary containing information on the fields of a framebulk
    """
    return (
        fields_dict[KW_MAP].encode('utf-8') + b'\x00' +
        fields_dict[KW_KART_NAME].encode('utf-8') + b'\x00' +
        struct.pack('3ic',
            fields_dict[KW_NUM_AI],
            fields_dict[KW_NUM_LAPS],
            fields_dict[KW_DIFFICULTY],
            b'\x01' if fields_dict[KW_QUICK_RESET] else b'\x00'
        )
    )


def parse_header(lines: List[Tuple[int, str]]) -> dict:
    """Parse script header and convert to bytes.

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
        define_field(KW_QUICK_RESET),
    ))

    fields_dict = {}

    # go through each line and add any header field values to the dict
    for m, line_num, line in ((re.match(header_fields_regex, line[1]), *line) for line in lines if line):
        if m:
            for k, v in m.groupdict().items():
                if v is not None:
                    fields_dict[k] = v
        else:
            print(f'Warning, unrecognized syntax on line {line_num}: "{line}", ignoring.')

    # validate header values

    # check that all mandatory keys exist
    for k in {KW_MAP, KW_KART_NAME, KW_NUM_LAPS, KW_DIFFICULTY} - fields_dict.keys():
        print(f"Value for '{k}' not found.")
        exit(1)

    def validate_field(key: str, convert_func: Callable, validate_func: Callable) -> bool:
        """Converts a field from fields_dict to a different value and checks if that value makes sense.

        key - header key
        convert_func - tries to convert the value to a new value (e.g. str -> int)
        validate_func - returns true if the new value is valid
        """
        try:
            new_val = convert_func(fields_dict[key])
            success = validate_func(new_val)
            if success:
                fields_dict[key] = new_val
            return success
        except:
            return False

    # num_laps should be -1 or positive int
    if not validate_field(KW_NUM_LAPS, lambda s: int(s), lambda n: n == -1 or n > 0):
        print(f"Invalid value '{fields_dict[KW_NUM_LAPS]}' for key '{KW_NUM_LAPS}', \
            should be a positive integer or -1 in special cases.")
        exit(1)

    # difficulty should be in [0,3]
    if not validate_field(KW_DIFFICULTY, lambda s: int(s), lambda n: 0 <= n <= 3):
        print(f"Invalid value '{fields_dict[KW_DIFFICULTY]}' for key '{KW_DIFFICULTY}', should be 0-3.")
        exit(1)

    # num_ai is currently unused but still sent, give it a default value of 0
    if KW_NUM_AI not in fields_dict:
        fields_dict[KW_NUM_AI] = '0'
    if not validate_field(KW_NUM_AI, lambda s: int(s), lambda n: n >= 0):
        print(f"Invalid value '{fields_dict[KW_NUM_AI]}' for key '{KW_NUM_AI}', should be at least 0.")
        exit(1)

    if fields_dict[KW_MAP] not in MAP_NAMES:
        print(f"Invalid map name: '{fields_dict[KW_MAP]}'. Here's a list of all valid maps:")
        print('\n'.join(sorted(MAP_NAMES)))
        exit(1)

    if fields_dict[KW_KART_NAME] not in KART_NAMES:
        print(f"Invalid kart name: '{fields_dict[KW_KART_NAME]}'. Here's a list of all valid kart names:")
        print('\n'.join(sorted(KART_NAMES)))
        exit(1)

    valid_bools = {
        **{f: False for f in ('f', 'false', '0')},
        **{t: True for t in ('t', 'true', '1')}
    }

    # quick_reset is optional, default value is false
    if KW_QUICK_RESET not in fields_dict:
        fields_dict[KW_QUICK_RESET] = 'false'
    if not validate_field(KW_QUICK_RESET, lambda s: valid_bools[s], lambda b: True):
        print(f"Invalid value '{fields_dict[KW_QUICK_RESET]}' for key '{KW_QUICK_RESET}', expected boolean.")
        exit(1)

    # map + kart_name + int(num_ai) + int(num_laps) + int(difficulty) + byte(quick_reset)
    return fields_dict

def encode_framebulks(framebulks: List[Framebulk]) -> bytes:
    """Converts list of framebulks into bytes

    Keyword arguments:
    framebulks -- list of Framebulk objects
    """
    fb_bytes = b''
    for framebulk in framebulks:
        fb_bytes += framebulk.encode()
    return fb_bytes


def parse_framebulks(lines: List[Tuple[int, str]]) -> List[Framebulk]:
    """Parse script framebulks and convert to bytes

    Keyword arguments:
    lines -- all lines in the script after the 'frames' keyword,
    these lines are assumed to have no leading/trailing whitespace
    """

    framebulks = []

    for line_num, line in lines:
        framebulks.append(Framebulk.from_script(line, line_num))

    return framebulks


def parse_script(tas_file: str) -> bytes:
    """parse TAS file

    Keyword arguments:
    tasFile -- TAS filename
    """

    def clean_line(line: str) -> str:
        """Cleans each line of the input TAS script. Removes any comments,
        unnecessary whitespace, and blank lines. Then converts to lower case."""
        return re.sub(r'\s+', ' ', line[:line.find("//")].strip().lower())

    with open(tas_file, 'r') as f:
        # lines is an int/string pair: line_number/line_content
        lines: List[Tuple[int, str]] = [
            y for y in enumerate((clean_line(x) for x in f.readlines()), start=1) if y[1]
        ]

    # find first line with 'frames' on it
    try:
        header_end_idx = next(i for i, line in enumerate(lines) if line[1] == KW_HEADER_END)
    except StopIteration:
        print(f"No '{KW_HEADER_END}' keyword found, not sure where header ends.")
        exit(1)

    header = parse_header(lines[:header_end_idx])
    framebulks = parse_framebulks(lines[header_end_idx+1:])

    return encode_header(header) + encode_framebulks(framebulks)


def get_script_path() -> str:
    """Parses command line arguments and retrieves a path. 
    If no path is given then the default path is used instead.

    Return:
    str -- String representing the path to the TAS script to be parsed
    """
    default = "./scripts/tasfile.peng"
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--path', type=str)
    args = parser.parse_args()
    if args.path is None:
        print(f"Notice: No path given. Using default path: '{default}'")
        return default
    return args.path


def main():
    # Get path to TAS script
    tas_script_path = get_script_path()
    path = pathlib.Path(tas_script_path)
    if not path.is_file():
        print("Error: File does not exist. Exiting...")
        exit(-1)
    script_bytes = parse_script(tas_script_path)

    # run the injector exe

    inj_name = "Injector.exe"
    pyl_name = "Payload.dll"

    official_path = "./"  # official release (same dir)
    deb_path = "../x64/Debug/"
    rel_path = "../x64/Release/"

    # official, debug, release
    build_files: List[List[str]] = [
        [path + name for name in (inj_name, pyl_name)]
        for path in (official_path, deb_path, rel_path)
    ]

    official_files, deb_files, rel_files = build_files

    official_exists, deb_exists, rel_exists = \
        [all(pathlib.Path(file).is_file() for file in build) for build in build_files]

    # Try official release path, on fail try the most recent build path.
    # If neither path exists, we have to trust that the user runs
    # the injector on their own.
    inj_path = None

    if official_exists:
        inj_path = official_path + inj_name
    else:
        print(f"{official_path + inj_name} not found, looking for vs builds.")
        if deb_exists and rel_exists:
            deb_mod_time = max(os.path.getmtime(file) for file in deb_files)
            rel_mod_time = max(os.path.getmtime(file) for file in rel_files)
            inj_path = (deb_path if deb_mod_time > rel_mod_time else rel_path) + inj_name
        elif deb_exists:
            inj_path = deb_path + inj_name
        elif rel_exists:
            inj_path = rel_path + inj_name
        else:
            print(f"Can't find {inj_name}, trying to connect")

    return_code = 0
    if inj_path:
        print(f"Running {inj_path}")
        return_code = os.system(str(pathlib.Path(inj_path)))
    # Open Client socket and send data to Payload if the injector ran successfully
    if return_code == 0:
        cl_sock = ClientSocket()
        cl_sock.start()
        cl_sock.send(script_bytes, MessageType.Script)


if __name__ == "__main__":
    main()
