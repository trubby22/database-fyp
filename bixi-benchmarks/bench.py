import pandas as pd
import numpy as np
import sqlite3
import duckdb
from time import time_ns
from bixi_pandas import go

num_warmup = 0
num_main = 1
# paths to input csvs (map from name to path)
vendor_input_file_paths = {
  'pandas': ...,
  'duckdb': ...,
  'sqlite': ...,
}

rand_names_paths = {
  "_1_64b": "/root/Documents/4-year/fyp-70011/data/random-data/_64b.csv",
  "_2_1mb": "/root/Documents/4-year/fyp-70011/data/random-data/_1mb.csv",
  "_3_10mb": "/root/Documents/4-year/fyp-70011/data/random-data/_10mb.csv",
  "_4_100mb": "/root/Documents/4-year/fyp-70011/data/random-data/_100mb.csv",
  "_5_1gb": "/root/Documents/4-year/fyp-70011/data/random-data/_1gb.csv",
  "_6_2gb": "/root/Documents/4-year/fyp-70011/data/random-data/_2gb.csv",
}

bixi_names_paths = {
  "bixi": "/root/Documents/4-year/fyp-70011/data/bixi-data/bixi-no-index-yes-colnames.csv",
}

# paths to output csvs
rand_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-rand-results.csv"
bixi_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-bixi-results.csv"

# pandas table to store timings

# queries maps (map from name to query
def data_in(vendor, path, table_name):
  if vendor == 'pandas':
    return load_pandas(table_name)
  elif vendor == 'duckdb':
    return load_duckdb(path, table_name)
  elif vendor == 'sqlite':
    return load_sqlite(path, table_name)

rand_queries = {
  "_1_data_in": data_in
}

def predict_duration_from_distance(vendor, path, table_name):
  df = data_in(vendor, path, table_name)
  go(df)

bixi_queries = {
  "_1_data_in": data_in,
  "_2_predict_duration_from_distance": predict_duration_from_distance
}

def load_pandas(path):
  return pd.read_csv(path)

def load_duckdb(path, table_name):
  with duckdb.connect(path) as con:
    df = con.execute(f"SELECT * FROM {table_name}").fetchdf()
    return df

def load_sqlite(path, table_name):
  with sqlite3.connect(path) as con:
    df = pd.read_sql(f"SELECT * FROM {table_name}", con)
    return df

# loader map
loaders = {
  'pandas': load_pandas,
  'duckdb': load_duckdb,
  'sqlite': load_sqltie
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



# rand table
# bixi