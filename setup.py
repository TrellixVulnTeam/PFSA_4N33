from distutils.core import setup
from Cython.Build import cythonize
from Cython.Compiler import Options
Options.embed = True
Options.embed = "main"
setup(
    ext_modules = cythonize("run.pyx")
)
