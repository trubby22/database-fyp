import pandas as pd
import numpy as np
import sqlite3
import duckdb

num_warmup = 0
num_main = 1
# paths to input csvs (map from name to path)
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

def data_in_rand():
  pass

def round_trip():
  pass

def materialise_columns():
  pass

def materialise_matrix():
  pass

def matrix_vector_product():
  pass

def matrix_matrix_product():
  pass

# queries maps (map from name to query)
rand_queries = {
  "_1_data_in": data_in_rand,
  "_2_round_trip": round_trip,
  "_3_materialise_columns": materialise_columns,
  "_4_materialise_matrix": materialise_matrix,
  "_5_matrix_vector_product": matrix_vector_product,
  "_6_matrix_matrix_product": matrix_matrix_product
}

def data_in_bixi():
  pass

def predict_duration_from_distance():
  pass

bixi_queries = {
  "_1_data_in": data_in_bixi,
  "_2_predict_duration_from_distance": predict_duration_from_distance
}

def load_pandas():
  pass

def load_duckdb():
  pass

def load_sqlite():
  pass

# loader map
loaders = {
  'pandas': load_pandas,
  'duckdb': load_duckdb,
  'sqlite': load_sqltie
}

# print elapsed time function

# benchmarking loop - function

# main



# rand table
# bixi