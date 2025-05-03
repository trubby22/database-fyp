# mypackage/setup.py
from setuptools import setup, find_packages

setup(
    name='rel_alg_numba',
    version='0.1',
    packages=find_packages(),
    install_requires=[
        'numba', 'numpy'
    ],
)
