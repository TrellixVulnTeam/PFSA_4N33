import os
import sys
import pathlib

def pathlistdisplay(directory):
    print("__________"+str(pathlib.Path(directory))+"__________")
    for x in range(len(os.listdir(directory))):
        print("|"+os.listdir(directory)[x])
current_dir = str(pathlib.Path(os.getcwd()))
try:
    a = sys.argv[1]+" "
    pth = True
except:
    pth = False
if pth == True:
    pathlistdisplay(sys.argv[1])
else:
    pathlistdisplay(current_dir)
