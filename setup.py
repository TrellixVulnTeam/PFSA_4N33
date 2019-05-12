import sys
from cx_Freeze import setup, Executable

# Dependencies are automatically detected, but it might need fine tuning.
build_exe_options = {"packages": ["os","pathlib","sys"], "excludes": ["tkinter"]}

# GUI applications require a different base on Windows (the default is for a
# console application).
base = None
if sys.platform == "win32":
    base = "Console"

setup(  name = "PFSA",
        version = "0.1",
        description = "Python File System Analyser",
        options = {"build_exe": build_exe_options},
        executables = [Executable("run.py", base=base)])
