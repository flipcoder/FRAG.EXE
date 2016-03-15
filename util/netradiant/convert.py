#!/usr/bin/env python2
import sys
import os
import shutil

m = os.path.splitext(sys.argv[1])[0]
obj_fn = m+".obj"
mtl_fn = m+".mtl"
old_obj_fn = "old_"+obj_fn
old_mtl_fn = "old_"+mtl_fn

if not os.path.exists(obj_fn) or not os.path.exists(mtl_fn):
    print "must exist"
    sys.exit(1)

shutil.copyfile(obj_fn, old_obj_fn)
shutil.copyfile(mtl_fn, old_mtl_fn)

mtl = open(old_mtl_fn,'r')
mtl_r = open(mtl_fn,'w')
for l in mtl:
    if l.startswith("newmtl "):
        mtl_r.write(l)
        mtl_r.write("map_Kd " + os.path.basename(l.split()[1]) + ".png\n\n")

