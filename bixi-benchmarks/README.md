The important files in this repo are:
* bench.py - benchmarks the following
    * the time taken to load in the BIXI Montreal dataset and calculate the linear regression task
    * the time taken to load data into a pandas DataFrame in Python from an ODBC of DuckDB and SQLite and also from a csv file using pandas
* datagen.py - generates random integer tables of the following sizes: 64 B, 1 MB, 10 MB, 100 MB, 1 GB, 2 GB
