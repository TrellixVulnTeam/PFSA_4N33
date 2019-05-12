import os
import sys

for arg in sys.argv:
    print(arg)
print("      ")
print("      ")
print("      ")
print("      ")
for x in range(len(os.listdir())):
    print(os.listdir()[x])
