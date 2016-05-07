#!/usr/bin/env python2
import sys
import os
import shutil
import json
import uuid

def increment_string(s):
    print s
    if s == "":
        s = "0"
        return
    num_s = ""
    while s[-1].isdigit():
        num_s = s[-1] + num_s
        s = s[:-1]
    if num_s == "":
        num_s = 0
    num_s = str(int(num_s) + 1)
    return s + num_s

# print increment_string('a')

SCALE = 0.025

m = os.path.splitext(sys.argv[1])[0]

# old_obj_fn = m+".obj"
# old_mtl_fn = m+".mtl"
# obj_fn = "new_"+m+".obj"
# mtl_fn = "new_"+m+".mtl"
old_map_fn = m+".map"

# if not os.path.exists(old_obj_fn) or not os.path.exists(old_mtl_fn):
#     print "must exist"
#     sys.exit(1)

# shutil.copyfile(old_obj_fn, obj_fn)
# shutil.copyfile(old_mtl_fn, mtl_fn)

# mtl = open(old_mtl_fn,'r')
# mtl_r = open(mtl_fn,'w')
# for l in mtl:
#     if l.startswith("newmtl "):
#         tokens = l.split(' ')
#         mtl_r.write(tokens[0] + " " + os.path.basename(tokens[1]))
#         mtl_r.write("map_Kd " + os.path.basename(l.split()[1]) + ".png\n\n")

# obj = open(old_obj_fn,'r')
# obj_r = open(obj_fn,'w')
# for l in obj:
#     if l.startswith("usemtl "):
#         tokens = l.split(' ')
#         obj_r.write(tokens[0] + " " + os.path.basename(tokens[1]))
#     elif l.startswith("v "):
#         tokens = l.split(" ")[1:]
#         for i in range(0,len(tokens)):
#             tokens[i] = str(float(tokens[i]) * SCALE)
#         tokens[0] = str(-float(tokens[0]))
#         l = "v " + " ".join(tokens) + "\n"
#         obj_r.write(l)
#     elif l.startswith("vt "):
#         tokens = l.split(" ")[1:]
#         tokens[1] = str(1.0 - float(tokens[1]))
#         l = "vt " + " ".join(tokens) + "\n"
#         obj_r.write(l)
#     elif l.startswith("f "):
#         tokens = l.split(" ")[1:]
#         for i in range(0,len(tokens)//2):
#             tokens[i], tokens[len(tokens)-2-i] = tokens[len(tokens)-2-i], tokens[i]
#         l = "f " + " ".join(tokens) + "\n"
#         obj_r.write(l)
#     else:
#         obj_r.write(l)

qmap = open(old_map_fn,'r')
jsonmap = {"nodes":{}}
node = None
for l in qmap:
    if l.startswith('\"origin\"'):
        if node:
            tokens = l.replace('\"','').split(' ')[1:]
            node["matrix"] = [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                -float(tokens[0]) * SCALE,
                float(tokens[2]) * SCALE,
                float(tokens[1]) * SCALE,
                1.0
            ]
    elif l.startswith('\"classname\" \"light\"'):
        node = {"type":"light"}
        tokens = l.split(' ')[1:]
        for i in range(0,len(tokens)):
            tokens[i] = tokens[i].replace('\"','')
        node["light"] = "point"
    elif l.startswith('\"classname\" \"misc_model\"'):
        node = {"type":"mesh"}
    # elif l.lower().startswith('\"classname\" \"spawn'):
    #     node = {
    #         "type": "empty",
    #         "name": "spawn"
    #     }
    elif l.lower().startswith('\"classname\" '):
        tokens = l.split(' ')[1:]
        for i in range(0,len(tokens)):
            tokens[i] = tokens[i].replace('\"','').replace("\n","")
        node = {
            "type": "empty",
            "name": tokens[0]
        }
    elif l.startswith('\"model\" '):
        fn = os.path.basename(l.split(' ')[1].replace('\"',''))
        node = {"data":os.path.basename(fn.strip())}
    elif l.startswith('\"_color\" '):
        if node:
            tokens = l.replace('\"','').split(' ')[1:]
            node["color"] = [
                float(tokens[0]),
                float(tokens[1]),
                float(tokens[2])
            ]
    elif l.startswith('\"light\" '):
        if node:
            if node["type"] == "light":
                tokens = l.split(" ")
                tokens = tokens[1:]
                for i in range(0,len(tokens)):
                    tokens[i] = tokens[i].replace('\"','')
                node["distance"] = float(tokens[0]) * SCALE
    elif l.startswith("}") and node:
        print node
        name = ""
        try:
            name = node['name']
        except:
            try:
                name = node['type'] + ".0"
            except:
                name = "empty.0"
        while name in jsonmap['nodes']:
            name = increment_string(name)
        print name
        jsonmap["nodes"][name] = node
        node = None

with open("new_"+m+".json", "w") as f:
    f.write(json.dumps(jsonmap, sort_keys=True, indent=4))
 
