import pandas as pd
import numpy as np
import sqlite3
import duckdb
from time import time_ns
from bixi_pandas import go

num_warmup = 0
num_main = 1
# paths to input csvs (map from name to path)
vendors = [
  # 'pandas',
  # 'duckdb',
  'sqlite',
]

vendor_input_paths = {
  'pandas': '/root/Documents/4-year/fyp-70011/data/csv/',
  'duckdb': '/root/Documents/4-year/fyp-70011/data/duckdb.db',
  'sqlite': '/root/Documents/4-year/fyp-70011/data/sqlite.db',
}

rand_names = [
  '_64b',
  '_1mb',
  # '_10mb',
  # '_100mb',
  # '_1gb',
  # '_2gb',
]

bixi_names = [
  'bixi'
]

# paths to output csvs
rand_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-rand-results.csv"
bixi_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-bixi-results.csv"

# queries maps (map from name to query
def data_in(vendor, table_name):
  if vendor == 'pandas':
    return load_pandas(table_name)
  elif vendor == 'duckdb':
    return load_duckdb(table_name)
  elif vendor == 'sqlite':
    return load_sqlite(table_name)

rand_queries = {
  "data_in": data_in
}

def predict_duration_from_distance(vendor, table_name):
  df = data_in(vendor, table_name)
  print(df)
  print(df.dtypes)
  # go(df)

bixi_queries = {
  # "data_in": data_in,
  "predict_duration_from_distance": predict_duration_from_distance,
}

def load_pandas(table_name):
  path_prefix = vendor_input_paths['pandas']
  return pd.read_csv(f'{path_prefix}{table_name}.csv')

def load_duckdb(table_name):
  with duckdb.connect(vendor_input_paths['duckdb']) as con:
    df = con.execute(f"SELECT * FROM {table_name}").fetchdf()
    return df

def load_sqlite(table_name):
  with sqlite3.connect(vendor_input_paths['sqlite']) as con:
    df = pd.read_sql(f"SELECT * FROM {table_name}", con)
    return df

# loader map
loaders = {
  'pandas': load_pandas,
  'duckdb': load_duckdb,
  'sqlite': load_sqlite,
}

# print elapsed time function

def print_elapsed_time(duration_ns):
  ns = duration_ns
  us = ns / (10 ** 3)
  ms = ns / (10 ** 6)
  s = ns / (10 ** 9)
  print(f'{s} [s]')
  print(f'{ms} [ms]')
  print(f'{us} [µs]')
  print(f'{ns} [ns]')

# pandas table to store timings



# main
# benchmarking loop - function
def bench_loop(table_names, queries, results_path):
  vendor_query = []
  for vendor in vendors:
    for query_name in queries:
      vendor_query.append(f'{vendor}-{query_name}')
  timings = pd.DataFrame(index=range(len(table_names)), columns=['table-name', *vendor_query])

  for vendor in vendors:
    i = 0
    for table_name in table_names:
      for query_name, query in queries.items():
        start = time_ns()
        query(vendor, table_name)
        stop = time_ns()
        delta = stop - start
        print(vendor, table_name, query_name)
        print_elapsed_time(delta)
        print()
        timings.loc[i, f'{vendor}-{query_name}'] = delta
      timings.loc[i, 'table-name'] = table_name
      i += 1

  timings.to_csv(results_path, index=False)

# bench_loop(rand_names, rand_queries, rand_results_path)
bench_loop(bixi_names, bixi_queries, bixi_results_path)

# rand table
# bixi