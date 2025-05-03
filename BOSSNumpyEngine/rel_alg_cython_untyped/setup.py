from setuptools import setup, Extension
from Cython.Build import cythonize
import numpy as np

extensions = [
    Extension(
        name="rel_alg_cython_untyped",
        sources=["rel_alg_cython_untyped.pyx"],
        include_dirs=[np.get_include()]  # This is the key line
    ),
]

setup(
    ext_modules=cythonize(extensions),
)