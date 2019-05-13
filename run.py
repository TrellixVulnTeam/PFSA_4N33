#Imports
import os
import sys
import pathlib
#Easy to use clear command
clear = lambda: os.system('cls')
directory = ""
def pathlistdisplay(directory):
    directory = pathlib.Path(directory)
    #Cool tree listing...
    print("__________"+str(pathlib.Path(directory))+"__________")
    for x in range(len(os.listdir(str(directory)))):
        print("|-"+os.listdir(str(directory))[x])

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
    if sys.argv[1] == "explorer":
        cancellisting = True
        try:
            a = sys.argv[2]+" "
            directory_precised = True
        except:
            directory_precised = False
        if directory_precised == True:
            pathlistdisplay(sys.argv[2])
        if directory_precised == False:
            pathlistdisplay(current_dir)
        
        while 1:
            clear()
            directory = pathlib.Path(directory)
            pathlistdisplay(directory)
            command = input(": ")
            if command == "exit":
                break
            if command == "parent":
                directory = pathlib.Path(directory.parent)
            if command[0] == "g":
                if command[1] == "o":
                    folder = str(directory)+"\\"
                    for j in range(len(command)-3):
                        folder = folder+command[j+3]
                    directory = pathlib.Path(folder)
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
