from setuptools import setup
from Cython.Build import cythonize

setup(
    name='Relational algebra operators',
    ext_modules=cythonize("rel_alg.pyx"),
)