The important files in this repo are:
* rel_alg_cython_untyped/rel_alg_cython_untyped.pyx - implements the relational algebra operators: project, select, equi-join, aggregate
* Source/BOSSNumpyEngine.cpp - implements the adapter between BOSS and Cython, walks the query tree, converts data format between a BOSS table and NumPy array-based Python table, dispatches to Cython operators
