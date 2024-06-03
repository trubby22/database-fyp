import duckdb
import pandas as pd
from bixi_pandas import go

with duckdb.connect('~/Documents/4-year/fyp-70011/bixi-data/duckdb-bixi.db') as con:
    df = con.execute("SELECT * FROM bixi").fetchdf()
    go(df)
