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

test_path = "./test/tasfile"

def processTASFields(fields):
    """process each field and extract information


    Keyword arguments:
    fields -- list containing all fields
    """

def processTASLines(data):
    """process lines from TAS script


    Keyword arguments:
    data -- list containing lines from TAS script
    """

    print(data)

    # search for map
    results = re.findall("map <([A-z ]+)>", data[0])
    if not results: # check if list is empty
        print("Warning: Map not found.")

    # begin parsing framebulks
    if not "frames" in data[1]:
        print("Error: Did not find start of framebulks. Exiting...")
        exit(-1)

    # syntax for framebulks:
    # --|---|-|0|
    # - indicate a field
    # 0 indicates number of ticks
    line_num = 3
    for line in data[2:]:
        line_num += 1
        fields = [x for x in line.split("|") if not x.isspace()] # removes spaces from list elems
        print("fields: ", fields)
        if len(fields) != 4:
            print("Warning: Error parsing framebulk (line " + str(line_num) + "). Skipping...")
            continue
        acc =   fields[0]
        misch = fields[1]
        ang =   fields[2]
        ticks = fields[3]
        processTASFields(fields)

def removeComments(string):
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



    processTASLines(data)

    file.close()



    


def main():
    """
    """
    global test_path
    parseTAS(test_path)

if __name__ == "__main__":
    main()