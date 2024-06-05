import pandas as pd
import numpy as np
import sqlite3

from bixi_pandas import go

db_path = '/mnt/ubuntu-image-repos/bixi-benchmarks/bixi.db'
conn = sqlite3.connect(db_path)
query = "SELECT * FROM bixi"
trips = pd.read_sql(query, conn, dtype={
    'duration_sec': np.int64,
    'longitude_x': np.float64,
    'latitude_x': np.float64,
    'longitude_y': np.float64,
    'latitude_y': np.float64
})
conn.close()

go(trips)
