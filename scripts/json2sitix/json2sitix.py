# simple script that turns JSON files into .stx object files
import json, sys


inp = json.loads(open(sys.argv[1]).read())
out = open(sys.argv[2], "w+")
tablevel = 0
pretty = "-p" in sys.argv

def handleObject(obj): # recursively convert json objects to sitix data
    global out
    global pretty
    global tablevel
    if type(obj) == dict:
        for key in obj.keys():
            if pretty:
                out.write("\t" * tablevel)
            tablevel += 1
            out.write("[=" + key + "-]")
            if pretty:
                out.write("\n")
            if type(obj[key]) == dict or type(obj[key]) == list:
                handleObject(obj[key])
            else:
                if pretty:
                    out.write("\t" * tablevel)
                out.write(str(obj[key]))
                if pretty:
                    out.write("\n")
            tablevel -= 1
            if pretty:
                out.write("\t" * tablevel)
            out.write("[/]")
            if pretty:
                out.write("\n")
    elif type(obj) == list:
        for item in obj:
            if pretty:
                out.write("\t" * tablevel)
            tablevel += 1
            out.write("[=+-]")
            if pretty:
                out.write("\n")
            if type(item) == dict or type(item) == list:
                handleObject(item)
            else:
                if pretty:
                    out.write("\t" * tablevel)
                out.write(str(item))
                if pretty:
                    out.write("\n")
            tablevel -= 1
            if pretty:
                out.write("\t" * tablevel)
            out.write("[/]")
            if pretty:
                out.write("\n")
    else:
        print("ERROR: Bad data at handleObject!")

handleObject(inp)