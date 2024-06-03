import pandas as pd
import pymonetdb
import numpy as np

from bixi_pandas import go

conn = pymonetdb.connect('mydatabase')
query = 'SELECT * FROM bixi'
df = pd.read_sql(query, conn, dtype={
    'duration_sec': np.int64,
    'longitude_x': np.float64,
    'latitude_x': np.float64,
    'longitude_y': np.float64,
    'latitude_y': np.float64
})
conn.close()
go(df)
