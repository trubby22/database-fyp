import pandas as pd
import numpy as np

rng = np.random.default_rng()
num_cols = 8
sizeof_dtype_bytes = 8
sizeof_row_bytes = num_cols * sizeof_dtype_bytes
columns = [f"c{i}" for i in range(1, num_cols + 1)]
table_sizes_bytes = [
    int(64),  # 64 b
    int(1e6), # 1 mb
    int(1e7), # 10 mb
    int(1e8), # 100 mb
    int(1e9), # 1 gb
    int(2e9), # 2 gb
]

table_sizes_names = [
    '_64b',
    '_1mb',
    '_10mb',
    '_100mb',
    '_1gb',
    '_2gb',
]

path_prefix = '/root/Documents/4-year/fyp-70011/data/random-data/'

for i in range(len(table_sizes_bytes)):
    table_size_bytes = table_sizes_bytes[i]
    table_name = table_sizes_names[i]

    table_size_bytes -= (table_size_bytes % sizeof_row_bytes)
    table_num_elems = table_size_bytes // sizeof_dtype_bytes
    num_rows = table_num_elems // num_cols

    data = rng.uniform(0, 100, (num_rows, num_cols))
    df = pd.DataFrame(data, columns=columns, dtype=np.float64)
    df.to_csv(f"{path_prefix}{table_name}.csv", index=False)

    print(f'done with {table_name}')
