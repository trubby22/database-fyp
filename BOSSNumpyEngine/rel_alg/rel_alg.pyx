import numpy as np
import cython
import math

def say_hello_to(name):
    print(f"Hello {name}!")

def project(dict[str, list[np.ndarray]] table, list[str] col_names):
    return {col_name : table[col_name] for col_name in col_names}

# works on materialised columns
def select(table, col_names, boolean_ops, vals):
    res_table = {}
    for i in range(len(col_names)):
        col_name = col_names[i]
        op = boolean_ops[i]
        val = vals[i]
        npy_arr = table[col_name]
        res_table[col_name] = boolean_op[op](npy_arr, val)
    return res_table

# works on spans
def select_spans(table, col_names, boolean_ops, vals):
    res_table = {}
    for i in range(len(col_names)):
        col_name = col_names[i]
        op = boolean_ops[i]
        val = vals[i]
        list_of_npy_arrs = table[col_name]
        res_col = []
        for j in range(len(list_of_npy_arrs)):
            npy_arr = list_of_npy_arrs[j]
            res_span = boolean_op[op](npy_arr, val)
            res_col.append(res_span)
        res_table[col_name] = res_col
    return res_table

# accepts and returns tables /w materialised columns
def equi_join(table_1, table_2, col_names_1, col_names_2):
    keys_1 = [table_1[col_name] for col_name in col_names_1]
    keys_2 = [table_2[col_name] for col_name in col_names_2]
    ixs_1 = np.lexsort(keys_1)
    ixs_2 = np.lexsort(keys_2)
    table_1_sorted = table_1[ixs_1]
    table_2_sorted = table_2[ixs_2]
    res_ix_1 = []
    res_ix_2 = []
    i = 0
    j = 0
    j_start = 0
    while i < len(table_1_sorted):
        j = j_start
        first = True
        while j < len(table_2_sorted):
            same = True
            for k in range(len(col_names_1)):
                col_name_1 = col_names_1[k]
                col_name_2 = col_names_2[k]
                elem_1 = table_1_sorted[col_name_1][i]
                elem_2 = table_2_sorted[col_name_2][j]
                if elem_1 != elem_2:
                    same = False
                    break
            if same:
                if first:
                    j_start = j
                    first = False
                res_ix_1.append(i)
                res_ix_2.append(j)
            if not same and not first:
                break
            j += 1
        i += 1
    
    res_ix_1_npy = np.array(res_ix_1)
    res_ix_2_npy = np.array(res_ix_2)
    table_2_joined = {col_name: table_2[col_name][res_ix_2_npy] for col_name in col_names_2}
    table_2_joined = {col_name: table_2[col_name][res_ix_2_npy] for col_name in col_names_2}
    return table_1_joined | table_2_joined

# accepts and returns tables /w materialised columns
def aggregate(table, key_col_names, reduction_func, reduction_col_name):
    keys = [table[col_name] for col_name in key_col_names]
    ixs = np.lexsort(keys)
    table_sorted = table[ixs]
    splits = []
    i = 0
    j = 0
    while i < len(table_sorted):
        j = i + 1
        while j < len(table_sorted):
            val_i = table_sorted[i]
            val_j = table_sorted[j]
            same = True
            for k in range(len(key_col_names)):
                col_name = key_col_names[k]
                elem_i = table_sorted[col_name][i]
                elem_j = table_sorted[col_name][j]
                if elem_1 != elem_2:
                    same = False
                    break
            if not same:
                break
            j += 1
        splits.append(j)
        i = j
    
    col_names = list(table.keys())
    table_split_up = {col_name: [x for x in np.split(table[col_name], splits) if len(x) > 0] for col_name in col_names}
    reduced_cols = [reduction_functions[reduction_func](x) for x in table[reduction_col_name]]
    table_reduced = {reduction_col_name: np.array(reduced_cols)}
    table_key = {col_name: np.array([x[0] for x in table_split_up[col_name][0]]) for col_name in key_col_names}
    return table_reduced | table_key

def materialise_into_columns(table):
    return {col_name: np.concatenate(table[col_name]) for col_name in table}

def split_into_spans(table, span_size):
    col_names = list(table.keys())
    col_name = col_names[0]
    num_splits = math.ceil(len(table[col_name]) / chunk_size)
    splits = np.array([(i + 1) * chunk_size for i in range(num_splits)])
    return {col_name: [x for x in np.split(table[col_name], splits) if len(x) > 0] for col_name in col_names}

reduction_functions = {
    'sum': lambda x: np.sum(x),
    'prod': lambda x: np.prod(x),
    'count': lambda x: x.size,
    'avg': lambda x: np.mean(x),
}

boolean_op = {
    '==': lambda x, y: x == y,
    '!=': lambda x, y: x != y,
    '<': lambda x, y: x < y,
    '<=': lambda x, y: x <= y,
    '>': lambda x, y: x > y,
    '>=': lambda x, y: x >= y,
}
