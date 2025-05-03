The important files in this repo are:
* rel_alg_cython_untyped/rel_alg_cython_untyped.pyx - implements the relational algebra operators: project, select, equi-join, aggregate
* Source/BOSSNumpyEngine.cpp - implements the adapter between BOSS and Cython, walks the query tree, converts data format between a BOSS table and NumPy array-based Python table, dispatches to Cython operators
* Tests/format-conversion-tests.cpp - tests round-trip of a table from BOSS to embedded Python and back to BOSS
* rel_alg_cython_untyped/rel_alg_cython_untyped_tests.py - tests the relational algebra operators of the NumPy relational algebra query execution engine in Python
* Tests/rel-alg-tests.cpp - tests the relational algebra operators of the NumPy relational algebra query execution engine in C++
