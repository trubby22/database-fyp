import pandas as pd
import numpy as np
import sqlite3

from bixi_pandas import go

# Path to your SQLite database
db_path = '/Users/piotrblaszyk/Documents/university/4-year/fyp-70011/bixi/bixi.db'

# Connect to the SQLite database
conn = sqlite3.connect(db_path)

# Query the database
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
