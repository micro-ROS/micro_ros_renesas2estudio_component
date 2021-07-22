#!/usr/bin/python

import sys
import json

f = open(sys.argv[1], "r")
colcon_meta =  json.load(f)
packages = colcon_meta["names"]

s = ""
for k,v in packages.items():
    s = s + k + " "

print(s)
