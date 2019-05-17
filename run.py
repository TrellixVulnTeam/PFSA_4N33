#Imports
import os
import sys
import pathlib
import imggenerate

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
    if sys.argv[1] == "img":
        cancellisting = True
        try:
            a = sys.argv[2]+" "
            directory_precised = True
        except:
            directory_precised = False
        try:
            a = sys.argv[3]+" "
            size_precised = True
        except:
            size_precised = False
        if directory_precised == True:
            if size_precised == True:
                imggenerate.generate(sys.argv[2],sys.argv[3]).save(sys.argv[2]+"\\"+"output.png")
            if size_precised == False:
                imggenerate.generate(sys.argv[2],40).save(sys.argv[2]+"\\"+"output.png")
        if directory_precised == False:
            if size_precised == True:
                imggenerate.generate(current_dir,sys.argv[3]).save(current_dir+"\\"+"output.png")
            if size_precised == False:
                imggenerate.generate(current_dir[2],40).save(current_dir+"\\"+"output.png")         
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
            try:
                pathlistdisplay(directory)
            except:
                print("Invalid Directory")
                directory = pathlib.Path(directory.parent)
            command = input(": ")
            if command == "exit":
                break
            if command == "parent":
                directory = pathlib.Path(directory.parent)
            if command[0] == "g":
                if command[1] == "o":
                    try:
                        folder = str(directory)+"\\"
                        for j in range(len(command)-3):
                            folder = folder+command[j+3]
                        directory = pathlib.Path(folder)
                    except:
                        print("Invalid Directory")
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
