#!/usr/bin/env python2
import sys
import os
import shutil

SCALE = 0.1

m = os.path.splitext(sys.argv[1])[0]
old_obj_fn = m+".obj"
old_mtl_fn = m+".mtl"
obj_fn = "new_"+m+".obj"
mtl_fn = "new_"+m+".mtl"
old_map_fn = m+".map"

if not os.path.exists(old_obj_fn) or not os.path.exists(old_mtl_fn):
    print "must exist"
    sys.exit(1)

shutil.copyfile(old_obj_fn, obj_fn)
shutil.copyfile(old_mtl_fn, mtl_fn)

mtl = open(old_mtl_fn,'r')
mtl_r = open(mtl_fn,'w')
for l in mtl:
    if l.startswith("newmtl "):
        mtl_r.write(l)
        mtl_r.write("map_Kd " + os.path.basename(l.split()[1]) + ".png\n\n")

obj = open(old_obj_fn,'r')
obj_r = open(obj_fn,'w')
for l in obj:
    if l.startswith("v "):
        tokens = l.split(" ")
        tokens = tokens[1:]
        for i in range(0,len(tokens)):
            tokens[i] = str(float(tokens[i]) * SCALE)
        l = "v " + " ".join(tokens) + "\n"
        obj_r.write(l)
    else:
        obj_r.write(l)

# qmap = open(old_map_fn,'r')
# for l in qmap:
#     print l

