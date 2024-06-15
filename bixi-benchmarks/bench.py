import pandas as pd
import numpy as np
import sqlite3
import duckdb
from time import time_ns
from bixi_pandas import go

num_warmup = 0
num_main = 1
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
  '_1mb',
  '_10mb',
  '_100mb',
  '_1gb',
  '_2gb',
]

bixi_names = [
  'bixi',
]

human_friendly_table_names = {
  '_64b' : '64 b',
  '_1mb' : '1 mb',
  '_10mb' : '10 mb',
  '_100mb' : '100 mb',
  '_1gb' : '1 gb',
  '_2gb' : '2 gb',
  'bixi' : 'bixi',
}

rand_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-rand-results.csv"
bixi_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/competition-bixi-results.csv"

def data_in(vendor, table_name):
  if vendor == 'pandas':
    return load_pandas(table_name)
  elif vendor == 'duckdb':
    return load_duckdb(table_name)
  elif vendor == 'sqlite':
    return load_sqlite(table_name)

rand_queries = {
  "data in": data_in
}

def predict_duration_from_distance(vendor, table_name):
  df = data_in(vendor, table_name)
  go(df)

bixi_queries = {
  "data in": data_in,
  "predict duration from distance": predict_duration_from_distance,
}

def load_pandas(table_name):
  path_prefix = vendor_input_paths['pandas']
  return pd.read_csv(f'{path_prefix}{table_name}.csv')

def load_duckdb(table_name):
  with duckdb.connect(vendor_input_paths['duckdb']) as con:
    df = con.execute(f"EXPLAIN ANALYZE SELECT * FROM {table_name}").fetchdf()
    return df

def load_sqlite(table_name):
  with sqlite3.connect(vendor_input_paths['sqlite']) as con:
    df = pd.read_sql(f"SELECT * FROM {table_name}", con)
    return df
    
loaders = {
  'pandas': load_pandas,
  'duckdb': load_duckdb,
  'sqlite': load_sqlite,
}

def print_elapsed_time(duration_ns):
  ns = duration_ns
  us = ns / (10 ** 3)
  ms = ns / (10 ** 6)
  s = ns / (10 ** 9)
  print(f'{s} [s]')
  print(f'{ms} [ms]')
  print(f'{us} [µs]')
  print(f'{ns} [ns]')

def s_to_ns(x):
  return x * (10 ** 9)

def bench_loop(table_names, queries, results_path):
  vendor_query = []
  for vendor in vendors:
    for query_name in queries:
      vendor_query.append(f'{vendor} {query_name}')
  timings = pd.DataFrame(index=range(len(table_names)), columns=['table name', *vendor_query])

  for vendor in vendors:
    i = 0
    for table_name in table_names:
      for query_name, query in queries.items():
        # warmup
        warmup_time_s = 3
        warmup_iters = 1
        completed_iters = 0

        start = time_ns()
        planned_end = start + s_to_ns(warmup_time_s)
        while completed_iters < warmup_iters or time_ns() < planned_end:
          query(vendor, table_name)
          completed_iters += 1

        # test
        test_time_s = 10
        test_iters = 3
        completed_iters = 0

        start = time_ns()
        planned_end = start + s_to_ns(test_time_s)
        while completed_iters < test_iters or time_ns() < planned_end:
          query(vendor, table_name)
          completed_iters += 1
        stop = time_ns()

        delta = (stop - start) / completed_iters
        print(vendor, table_name, query_name)
        print_elapsed_time(delta)
        print()
        timings.loc[i, f'{vendor} {query_name}'] = delta
      timings.loc[i, 'table name'] = human_friendly_table_names[table_name]
      i += 1

  timings.to_csv(results_path, index=False)

bench_loop(rand_names, rand_queries, rand_results_path)
bench_loop(bixi_names, bixi_queries, bixi_results_path)
