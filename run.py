import os
import sys
from pathlib import Path




mypath = Path().absolute()



print(mypath)
print("Share a kiss with your sys:\n"+sys.copyright)
print(os.path.join(mypath, ""))

