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
import json
import re

test_path = "./test/tasfile"

def processTASFields(fields):
    """process each field and extract information returning
    it in dictionary form


    Keyword arguments:
    fields -- list containing all fields

    Return:
    Dictionary -- dictionary containing info on framebulk
    """
    acc     = fields[0]
    misc    = fields[1]
    ang     = fields[2]
    ticks   = fields[3]

    # process acceleration fields
    accelerate = True if acc[0] == 'a' else False
    decelerate = True if acc[1] == 'b' else False

    # process mischelleneous fields
    fire = True if misc[0] == 'f' else False
    nitro = True if misc[1] == 'n' else False
    skid = True if misc[2] == 's' else False

    # process turning angle
    angleMag = float(ang)

    # process ticks field
    numTicks = int(ticks)

    return {
        "accelerate"    : accelerate,
        "decelerate"    : decelerate,
        "fire"          : fire,
        "nitro"         : nitro,
        "skid"          : skid,
        "angleMag"      : angleMag,
        "numTicks"      : numTicks
    }

def processTASLines(data):
    """process lines from TAS script and return information
    in dictionary format


    Keyword arguments:
    data -- list containing lines from TAS script

    Return:
    Dictionary -- dictionary containing all data in parsed TAS script
    """
    tasData = {}
    print(data)

    # search for map
    results = re.findall("map <([A-z ]+)>", data[0])
    if not results: # check if list is empty
        print("Warning: Map not found.")
    tasData['map'] = results[0]
    # begin parsing framebulks
    if not "frames" in data[1]:
        print("Error: Did not find start of framebulks. Exiting...")
        exit(-1)
    tasData['frame_bulks'] = []
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
        tasData['frame_bulks'].append(processTASFields(fields))

    return tasData

def removeComments(string): # literally hear just to remove comments :P
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

    with open('tasData.json', 'w') as file:
        json.dump(tasInfo, file)
    

if __name__ == "__main__":
    main()