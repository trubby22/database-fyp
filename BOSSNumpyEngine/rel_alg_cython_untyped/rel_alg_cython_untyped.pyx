# distutils: define_macros=NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION

import cython
cimport cython
import numpy as np
cimport numpy as cnp
import math

cnp.import_array()

def say_hello_to(name):
    maybe_log_one(f"Hello {name}!")

def project(
    table, 
    unary_ops, unary_col_names_input, unary_col_names_output, 
    binary_col_names_input_1, binary_ops, binary_col_names_input_2, binary_col_names_output,
    final_col_names, final_col_renames
    ):
    maybe_log_one('project start')
    table = materialise_into_columns(table)

    for i in range(len(unary_ops)):
        col_name = unary_col_names_input[i]
        op = unary_ops[i]
        res_name = unary_col_names_output[i]
        npy_arr = table[col_name]
        res = reduction_functions[op](npy_arr)
        table[res_name] = np.array([res])
    
    for i in range(len(binary_ops)):
        col_name_1 = binary_col_names_input_1[i]
        col_name_2 = binary_col_names_input_2[i]
        op = binary_ops[i]
        res_name = binary_col_names_output[i]
        npy_arr_1 = table[col_name_1] if not is_numeric(col_name_1) else col_name_1
        npy_arr_2 = table[col_name_2] if not is_numeric(col_name_2) else col_name_2
        res = arithmetic_binary_op[op](npy_arr_1, npy_arr_2)
        table[res_name] = res

    res = {final_col_renames[i]: table[final_col_names[i]] for i in range(len(final_col_names))}
    maybe_log_one(res)
    maybe_log_one('project end')
    maybe_log_one()
    return res

# works on materialised columns
def select(table, key_col_names, boolean_ops, vals):
    maybe_log_one('select start')
    table = materialise_into_columns(table)

    col_names = list(table.keys())
    bools = np.full(len(table[col_names[0]]), True)
    for i in range(len(key_col_names)):
        col_name = key_col_names[i]
        op = boolean_ops[i]
        val = vals[i]
        npy_arr = table[col_name]
        bools &= boolean_op[op](npy_arr, val)
    res = {col_name: table[col_name][ : len(bools)][bools] for col_name in col_names}
    maybe_log_one(res)
    maybe_log_one('select end')
    maybe_log_one()
    return res

# accepts and returns tables /w materialised columns
def equi_join(table_1, table_2, key_col_names_1, key_col_names_2):
    maybe_log_one('equi_join start')
    table_1 = materialise_into_columns(table_1)
    table_2 = materialise_into_columns(table_2)

    col_names_1 = list(table_1.keys())
    col_names_2 = list(table_2.keys())
    keys_1 = [table_1[col_name] for col_name in key_col_names_1]
    keys_2 = [table_2[col_name] for col_name in key_col_names_2]
    sort_ixs_1 = np.lexsort(keys_1)
    sort_ixs_2 = np.lexsort(keys_2)
    table_1_sorted = {col_name: table_1[col_name][sort_ixs_1] for col_name in col_names_1}
    table_2_sorted = {col_name: table_2[col_name][sort_ixs_2] for col_name in col_names_2}
    res_ix_1 = []
    res_ix_2 = []
    i = 0
    j = 0
    j_start = 0
    while i < len(table_1_sorted[key_col_names_1[0]]):
        j = j_start
        first = True
        while j < len(table_2_sorted[key_col_names_2[0]]):
            same = True
            for k in range(len(key_col_names_1)):
                col_name_1 = key_col_names_1[k]
                col_name_2 = key_col_names_2[k]
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
    table_1_joined = {col_name: table_1_sorted[col_name][res_ix_1_npy] for col_name in col_names_1}
    table_2_joined = {col_name: table_2_sorted[col_name][res_ix_2_npy] for col_name in col_names_2}
    res = table_1_joined | table_2_joined
    maybe_log_one(res)
    maybe_log_one('equi_join end')
    maybe_log_one()
    return res

# accepts and returns tables /w materialised columns
def aggregate(table, key_col_names, reduction_funcs, input_reduction_col_names, output_reduction_col_names):
    maybe_log_one('aggregate start')
    table = materialise_into_columns(table)

    col_names = list(table.keys())
    key_cols = [table[col_name] for col_name in key_col_names]
    sort_ixs = np.lexsort(key_cols)
    table_sorted = {col_name: table[col_name][sort_ixs] for col_name in col_names}
    splits = []
    i = 0
    j = 0
    while i < len(table_sorted[col_names[0]]):
        j = i + 1
        while j < len(table_sorted[col_names[0]]):
            same = True
            for k in range(len(key_col_names)):
                col_name = key_col_names[k]
                elem_i = table_sorted[col_name][i]
                elem_j = table_sorted[col_name][j]
                if elem_i != elem_j:
                    same = False
                    splits.append(j)
                    break
            if not same:
                break
            j += 1
        i = j
    
    table_split_up = {
        col_name: [x for x in np.split(table_sorted[col_name], splits) if len(x) > 0] 
        for col_name in col_names
    }
    table_reduced = {}
    for i in range(len(reduction_funcs)):
        reduction_func = reduction_funcs[i]
        input_reduction_col_name = input_reduction_col_names[i]
        output_reduction_col_name = output_reduction_col_names[i]
        reduced_col = [reduction_functions[reduction_func](x) for x in table_split_up[input_reduction_col_name]]
        table_reduced[output_reduction_col_name] = np.array(reduced_col)
    table_key = {col_name: np.array([x[0] for x in table_split_up[col_name]]) for col_name in key_col_names}
    res = table_key | table_reduced
    maybe_log_one(res)
    maybe_log_one('aggregate end')
    maybe_log_one()
    return res

def materialise_into_columns(table):
    return {col_name: np.concatenate([table[col_name]]).ravel() for col_name in table.keys()}

def split_into_spans(table, span_size):
    col_names = list(table.keys())
    num_splits = math.ceil(len(table[col_names[0]]) / span_size)
    splits = np.array([(i + 1) * span_size for i in range(num_splits)])
    return {col_name: [x for x in np.split(table[col_name], splits) if len(x) > 0] for col_name in col_names}

def is_numeric(s):
    try:
        int(s)
        return True
    except ValueError:
        pass
    
    try:
        float(s)
        return True
    except ValueError:
        pass
    
    return False

def maybe_log_one(x=None):
    if False:
        print(x)

reduction_functions = {
    'sum': lambda x: np.sum(x),
    'prod': lambda x: np.prod(x),
    'count': lambda x: x.size,
    'avg': lambda x: np.mean(x),
    'max': lambda x: np.max(x),
    'min': lambda x: np.min(x),
}

boolean_op = {
    '==': lambda x, y: x == y,
    '!=': lambda x, y: x != y,
    '<': lambda x, y: x < y,
    '<=': lambda x, y: x <= y,
    '>': lambda x, y: x > y,
    '>=': lambda x, y: x >= y,
}

arithmetic_binary_op = {
    '+': lambda x, y: x + y,
    '-': lambda x, y: x - y,
    '*': lambda x, y: x * y,
    '/': lambda x, y: x / y,
}

# unit tests
if __name__ == '__main__':
    table_1 = {
        'col1': np.array([1, 2, 3]),
        'col2': np.array([0.8, 3.14, 2.42]),
        'col3': np.array([0, 0, 1]),
    }
    project_res = project(table_1, ['col2', 'col3'])
    project_expected = {'col2': np.array([0.8 , 3.14, 2.42]), 'col3': np.array([0, 0, 1])}
    print('project_res')
    print(project_res)
    print('project_expected')
    print(project_expected)
    print()

    select_res = select(table_1, ['col3', 'col1'], ['==', '<'], [0, 1.5])
    select_expected = {'col1': np.array([1]), 'col2': np.array([0.8]), 'col3': np.array([0])}
    print('select_res')
    print(select_res)
    print('select_expected')
    print(select_expected)
    print()

    table_2 = {
        'col1': np.array([10, 5, 1]),
        'col2': np.array([3, 3, 4]),
        'col3': np.array([6, 7, 8]),
    }
    table_3 = {
        'col1': np.array([10, 5, 2]),
        'col2': np.array([5, 3, 4]),
        'col4': np.array([9, 2, 1]),
    }
    join_res = equi_join(table_2, table_3, ['col1', 'col2'], ['col1', 'col2'])
    join_expected = {'col1': np.array([5]), 'col2': np.array([3]), 'col3': np.array([7]), 'col4': np.array([2])}
    print('join_res')
    print(join_res)
    print('join_expected')
    print(join_expected)
    print()

    table_4 = {
        'col1': np.array([0, 1, 0, 1, 0, 1, 0, 1]),
        'col2': np.array([0, 0, 1, 1, 0, 0, 1, 1]),
        'col3': np.array([1, 2, 3, 4, 1, 2, 3, 4]),
    }
    aggregate_res = aggregate(table_4, ['col1', 'col2'], 'sum', 'col3')
    aggregate_expected = {'col1': np.array([0, 1, 0, 1]), 'col2': np.array([0, 0, 1, 1]), 'col3': np.array([2, 4, 6, 8])}
    print('aggregate_res')
    print(aggregate_res)
    print('aggregate_expected')
    print(aggregate_expected)
    print()

    table = {
        'col1': [
            np.array([1, 2], dtype=np.int32),
            np.array([3, 4], dtype=np.int32),
        ], 
        'col2': [
            np.array([5 ,6], dtype=np.int32),
            np.array([7, 8], dtype=np.int32),
        ],
    }
    wrapper = {
        'table': table,
        'matrix': None
    }

    table = {
        'col1': [
            np.array([1, 2, 3, 4], dtype=np.int32),
        ], 
        'col2': [
            np.array([5 ,6, 7, 8], dtype=np.int32),
        ],
    }
    wrapper = {
        'table': table,
        'matrix': None
    }

    matrix = np.array([
        [1, 2, 3, 4],
        [5, 6, 7, 8],
    ], dtype=np.int32)
    wrapper = {
        'table': None,
        'matrix': matrix
    }
    