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
  'pandas',
  'duckdb',
  'sqlite',
]

vendor_input_paths = {
  'pandas': '/root/Documents/4-year/fyp-70011/data/csv/',
  'duckdb': '/root/Documents/4-year/fyp-70011/data/duckdb.db',
  'sqlite': '/root/Documents/4-year/fyp-70011/data/sqlite.db',
}

rand_names = [
  '_64b',
  # '_1mb',
  # '_10mb',
  # '_100mb',
  # '_1gb',
  # '_2gb',
]

bixi_name = 'bixi-no-index-yes-colnames'

# paths to output csvs
rand_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-rand-results.csv"
bixi_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-bixi-results.csv"

# pandas table to store timings

# queries maps (map from name to query
def data_in(vendor, table_name):
  if vendor == 'pandas':
    return load_pandas(table_name)
  elif vendor == 'duckdb':
    return load_duckdb(table_name)
  elif vendor == 'sqlite':
    return load_sqlite(table_name)

rand_queries = {
  "_1_data_in": data_in
}

def predict_duration_from_distance(vendor, table_name):
  df = data_in(vendor, table_name)
  go(df)

bixi_queries = {
  "_1_data_in": data_in,
  "_2_predict_duration_from_distance": predict_duration_from_distance
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

# benchmarking loop - function

# main

for vendor in vendors:
  for table_name in rand_names:
    for query in rand_queries:
      start = time_ns()
      query(vendor, table_name)
      stop = time_ns()
      delta = stop - start
      print(vendor, table_name, query)
      print_elapsed_time(delta)
      print()

# rand table
# bixi