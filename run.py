import os
import sys
import pathlib


current_dir = pathlib.Path(os.getcwd())
if sys.argv[0] == "path":
print("__________"+os.getcwd()+"__________")

for x in range(len(os.listdir(current_dir))):
    print("|"+os.listdir(current_dir)[x])
