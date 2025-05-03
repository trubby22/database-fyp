import pandas as pd
import pyodbc

# Define the connection string
conn_str = (
    "DRIVER={SQLite3};"
    "DATABASE=/Users/piotrblaszyk/Documents/university/4-year/fyp-70011/bixi/bixi.db;"
)

# Establish the connection
conn = pyodbc.connect(conn_str)

# Query the database and load the data into a Pandas DataFrame
query = "SELECT * FROM bixi"
df = pd.read_sql(query, conn)

# Print the DataFrame
print(df)

# Close the connection
conn.close()
