#Imports
import os
import sys
import pathlib
#Easy to use clear command
clear = lambda: os.system('cls')

def pathlistdisplay(directory):
    #Cool tree listing...
    print("__________"+str(pathlib.Path(directory))+"__________")
    for x in range(len(os.listdir(directory))):
        print("|-"+os.listdir(directory)[x])

def open(directory):
    os.system("start "+str(pathlib.Path(directory)))




current_dir = str(pathlib.Path(os.getcwd()))
#Testing for first argument...
try:
    a = sys.argv[1]+" "
    pth = True
except:
    pth = False
cancellisting = False
if pth == True:
    if sys.argv[1] == "open":
        cancellisting = True
        #Testing if the directory is precised... If not use current working directory
        try:
            a = sys.argv[2]+" "
            directory_precised = True
        except:
            directory_precised = False
        print("Opening...")
        if directory_precised == True:
            open(sys.argv[2])
        if directory_precised == False:
            open(current_dir)
    try:
        if cancellisting == True:
            pass
        else:
            pathlistdisplay(sys.argv[1])
    except:
        print("Invalid Directory")
    
    else:
        pass
else:
    try:
        pathlistdisplay(current_dir)
    except:
        print("Invalid Directory")
