import duckdb
import pandas as pd
from bixi_pandas import go

def foo():
    with duckdb.connect('/root/Documents/4-year/fyp-70011/data/bixi-data/duckdb-bixi.db') as con:
        df = con.execute("SELECT * FROM bixi").fetchdf()
        go(df)
        return df

    print(foo())

