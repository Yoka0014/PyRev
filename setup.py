from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from Cython.Build import cythonize

import numpy as np
  
class my_build_ext(build_ext):
    def build_extensions(self):
        if self.compiler.compiler_type == "unix":
            for e in self.extensions:
                e.extra_compile_args = ["-std=c++20", 
                                        "-march=native", 
                                        "-O2"]
        elif self.compiler.compiler_type == "msvc":
            for e in self.extensions:
                e.extra_compile_args = ["/std:c++20", 
                                        "/arch:SSE", "/arch:SSE2", "/arch:AVX2", 
                                        "/source-charset:utf-8",
                                        "-O2"]

        build_ext.build_extensions(self)

ext = Extension("pyrev.pyrev", 
                sources=["pyrev/pyrev.pyx", "pyrev/cpp/config.cpp", "pyrev/cpp/bitboard.cpp", "pyrev/cpp/flip.cpp", "pyrev/cpp/mobility.cpp", "pyrev/cpp/utils/random.cpp"],
                include_dirs=["pyrev/cpp", "pyrev/cpp/utils", np.get_include()], language="c++")
 
setup(
    name="pyrev", 
    ext_modules=cythonize([ext]),
    cmdclass={"build_ext": my_build_ext},
    packages=["pyrev"],
    package_data={"pyrev": ["py.typed", "*.pyi"]}
)