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
        res_table[col_name] = boolean_op(npy_arr, op, val)
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
            res_span = boolean_op(npy_arr, op, val)
            res_col.append(res_span)
        res_table[col_name] = res_col
    return res_table

def boolean_op(arr, op, val):
    match op:
        case '<':
            return arr < val
        case '>':
            return arr > val
        case '==':
            return arr == val
        case '!=':
            return arr != val
        case '<=':
            return arr <= val
        case '>=':
            return arr >= val
        case _:
            raise Exception()

# accepts and returns tables /w materialised columns
def equi_join(table_1, table_2, col_names_1, col_names_2, chunk_size):
    keys_1 = [table_1[col_name] for col_name in col_names_1]
    keys_2 = [table_2[col_name] for col_name in col_names_2]
    ixs_1 = np.lexsort(keys_1)
    ixs_2 = np.lexsort(keys_2)
    table_1_sorted = table_1_materialised[ixs_1]
    table_2_sorted = table_2_materialised[ixs_2]
    res_ix_1 = []
    res_ix_2 = []
    i = 0
    j = 0
    j_start = 0
    while i < len(table_1_sorted):
        j = j_start
        first = True
        while j < len(table_2_sorted):
            val_1 = table_1_sorted[i]
            val_2 = table_2_sorted[j]
            if i == j:
                if first:
                    j_start = j
                    first = False
                res_ix_1.append(i)
                res_ix_2.append(j)
            j += 1
        i += 1
    
    res_ix_1_npy = np.array(res_ix_1)
    res_ix_2_npy = np.array(res_ix_2)
    table_2_joined = {col_name: table_2[col_name][res_ix_2_npy] for col_name in col_names_2}
    table_2_joined = {col_name: table_2[col_name][res_ix_2_npy] for col_name in col_names_2}
    return table_1_joined | table_2_joined

def materialise_into_columns(table):
    return {col_name: np.concatenate(table[col_name]) for col_name in table}

def split_into_spans(table, span_size):
    col_names = list(table.keys())
    col_name = col_names[0]
    num_splits = math.ceil(len(table[col_name]) / chunk_size)
    splits = np.array([(i + 1) * chunk_size for i in range(num_splits)])[ : -1 ]
    return {col_name: np.split(table, splits) for col_name in col_names}

