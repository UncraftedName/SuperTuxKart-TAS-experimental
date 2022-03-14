import unittest
import sys
import pathlib
import re
from typing import List, Tuple
import struct

sys.path.append("../Parser")

import parser

class TestParser(unittest.TestCase):

    def test_header_simple(self):
        """This test makes sure the parser is correctly parsing the header
        """
        test_header = [
            (1, f"{parser.KW_MAP} = abyss"),
            (2, f"{parser.KW_KART_NAME} = tux"),
            (3, f"{parser.KW_NUM_LAPS} = 1"),
            (4, f"{parser.KW_DIFFICULTY} = 2")
        ]
        test_output = parser.parse_header(test_header)
        expected_output = {
            parser.KW_MAP : "abyss",
            parser.KW_KART_NAME : "tux",
            parser.KW_NUM_LAPS : 1,
            parser.KW_DIFFICULTY : 2,
            parser.KW_NUM_AI : 0,
            parser.KW_QUICK_RESET : False 
        }
        
        self.assertEqual(test_output, expected_output)

    def test_framebulk_simple(self):
        """This test makes sure the parser is correctly parsing framebulks
        """
        test_framebulk = [
            (0, "a-|---|0|100|"),
            (1, f"{parser.KW_PLAYSPEED} 3.0")
        ]
        test_output = parser.parse_framebulks(test_framebulk)
        expected_output = [
            parser.Framebulk(100, 0.0, parser.Framebulk.Flags(accel=True)),
            parser.Framebulk(0, 3.0, parser.Framebulk.Flags(set_speed=True))
        ]

        self.assertEqual(test_output, expected_output)

    def parser_setup(self, tas_file: str):
        """This method will do the setup for the 'test_parser_simple' test.
        It takes a tas file and parses each of the lines providing the
        necessary information for the test to run
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

        try:
            header_end_idx = next(i for i, line in enumerate(lines) if line[1] == parser.KW_HEADER_END)
        except StopIteration:
            print(f"No '{parser.KW_HEADER_END}' keyword found, not sure where header ends.")
            exit(1)
        return (lines, header_end_idx)

    def test_parser_simple(self):
        """This test runs the parser and checks each input on the file tasfile.peng
        """

        # test setup
        tas_path = "./scripts/tasfile.peng"
        tas_file = pathlib.Path(tas_path)
        lines, header_end_idx = self.parser_setup(tas_file)

        # begin testing header
        test_header_output = parser.parse_header(lines[:header_end_idx])
        expected_header_output = {
            parser.KW_MAP : "abyss",
            parser.KW_KART_NAME : "tux",
            parser.KW_NUM_LAPS : 1,
            parser.KW_DIFFICULTY : 0,
            parser.KW_NUM_AI : 0,
            parser.KW_QUICK_RESET : False 
        }
        self.assertEqual(test_header_output, expected_header_output)

        # begin testing framebulks
        framebulks = parser.parse_framebulks(lines[header_end_idx+1:])

        self.assertEqual(framebulks[0],  parser.Framebulk(0,   5.0, parser.Framebulk.Flags(set_speed=True)))
        self.assertEqual(framebulks[1],  parser.Framebulk(600, 0.0, parser.Framebulk.Flags()))        
        self.assertEqual(framebulks[2],  parser.Framebulk(740, 0.0, parser.Framebulk.Flags(accel=True)))   
        self.assertEqual(framebulks[3],  parser.Framebulk(80, -1.0, parser.Framebulk.Flags(accel=True)))        
        self.assertEqual(framebulks[4],  parser.Framebulk(190, 0.0, parser.Framebulk.Flags(accel=True)))        
        self.assertEqual(framebulks[5],  parser.Framebulk(0,   0.5, parser.Framebulk.Flags(set_speed=True)))  
        self.assertEqual(framebulks[6],  parser.Framebulk(60,  1.0, parser.Framebulk.Flags(accel=True))) 
        self.assertEqual(framebulks[7],  parser.Framebulk(140, 0.0, parser.Framebulk.Flags(accel=True)))  
        self.assertEqual(framebulks[8],  parser.Framebulk(50,  1.0, parser.Framebulk.Flags(accel=True)))    
        self.assertEqual(framebulks[9],  parser.Framebulk(90,  0.0, parser.Framebulk.Flags(accel=True))) 
        self.assertEqual(framebulks[10], parser.Framebulk(50,  1.0, parser.Framebulk.Flags(accel=True)))
        self.assertEqual(framebulks[11], parser.Framebulk(80,  0.0, parser.Framebulk.Flags(accel=True)))
        self.assertEqual(framebulks[12], parser.Framebulk(50,  1.0, parser.Framebulk.Flags(accel=True, nitro=True, skid=True)))
        self.assertEqual(framebulks[13], parser.Framebulk(200, 0.0, parser.Framebulk.Flags(accel=True, nitro=True)))
        self.assertEqual(framebulks[14], parser.Framebulk(0,   0.0, parser.Framebulk.Flags(set_speed=True)))
      
    def test_header_encoding(self):
        """This method tests that the parser is correctly encoding the header
        """
        test_header = {
            parser.KW_MAP : "abyss",
            parser.KW_KART_NAME : "tux",
            parser.KW_NUM_LAPS : 1,
            parser.KW_DIFFICULTY : 2,
            parser.KW_NUM_AI : 0,
            parser.KW_QUICK_RESET : False 
        }
        test_output = parser.encode_header(test_header)
        expected_output = (
            b'abyss\x00' +          # map
            b'tux\x00' +            # player
            b'\x00\x00\x00\x00' +   # AI count
            b'\x01\x00\x00\x00' +   # Lap number
            b'\x02\x00\x00\x00' +   # Difficulty
            b'\x00'                 # Quick reset
        )
        self.assertEqual(test_output, expected_output)

    def test_framebulk_encoding(self):
        """This method tests that the parser is correctly encoding framebulks
        """
        test_output_0 = parser.Framebulk(100, 0.0, parser.Framebulk.Flags(accel=True)).encode()
        test_output_1 = parser.Framebulk(0, 3.0, parser.Framebulk.Flags(set_speed=True)).encode()

        expected_output_0 = struct.pack('hhf', parser.Framebulk.Flags.FLAG_ACCEL, 100, 0.0)
        expected_output_1 = struct.pack('hhf', parser.Framebulk.Flags.FLAG_SET_SPEED, 0, 3.0)

        self.assertEqual(test_output_0, expected_output_0)
        self.assertEqual(test_output_1, expected_output_1)


if __name__ == '__main__':
    unittest.main()
