import sys
sys.path.append("/mnt/ubuntu-image-repos/BOSSNumpyEngine/rel_alg_cython")
import numpy as np
from rel_alg_cython import *
import pandas as pd
from time import time_ns

def char_to_ascii(char):
    """Convert a character to its ASCII code."""
    return ord(char) if isinstance(char, str) and len(char) == 1 else char

# def float_to_int(char):
#     """Convert a character to its ASCII code."""
#     if isinstance(char, float):

#     return int(char) if isinstance(char, float) else char

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

if __name__ == '__main__':
    df_in = pd.read_csv(
        '/mnt/ubuntu-image-repos/BOSSKernelBenchmarks/data/tpch_100MB/lineitem.tbl', sep='|', header=None)
    # print(df.head())
    
    # print(ascii_df.head())
    df = df_in[[2, 4, 9]]
    df = df.fillna(0)
    df = df.map(char_to_ascii)
    df = df.astype(int)
    numpy_matrix = df.to_numpy(dtype=int).T.copy()
    print(numpy_matrix)
    print()

    # [[ 4  9  5 ...  3  1  1]
    # [17 36  8 ... 43 37 41]
    # [79 79 79 ... 79 79 70]]

# select
#     l_suppkey,
#     sum(l_quantity),
#     l_returnflag
# from
#     lineitem
# group by
#     l_suppkey,
#     l_returnflag

    res = aggregate_matrix(numpy_matrix, [0, 2], 'sum', 1)
    print(res)

    # warmup
    warmup_time_s = 3
    warmup_iters = 1
    completed_iters = 0

    start = time_ns()
    planned_end = start + s_to_ns(warmup_time_s)
    while completed_iters < warmup_iters or time_ns() < planned_end:
        res = aggregate_matrix(numpy_matrix, [0, 2], 'sum', 1)
        completed_iters += 1

    # test
    test_time_s = 10
    test_iters = 3
    completed_iters = 0

    start = time_ns()
    planned_end = start + s_to_ns(test_time_s)
    while completed_iters < test_iters or time_ns() < planned_end:
        res = aggregate_matrix(numpy_matrix, [0, 2], 'sum', 1)
        completed_iters += 1
    stop = time_ns()

    delta = (stop - start) / completed_iters
    print_elapsed_time(delta)
    print()



