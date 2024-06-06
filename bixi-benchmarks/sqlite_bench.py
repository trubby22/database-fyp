import pandas as pd
import numpy as np
import sqlite3

from bixi_pandas import go

db_path = '/root/Documents/4-year/fyp-70011/data/bixi-data/sqlite-bixi.db'
with sqlite3.connect(db_path) as con:
    query = "SELECT * FROM bixi"
    trips = pd.read_sql(query, con)
    print(trips)
    print(trips.dtypes)
