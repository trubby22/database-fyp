# distutils: define_macros=NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION

import cython
cimport cython
import numpy as np
cimport numpy as cnp
import math

cnp.import_array()
DTYPE = np.int64

ctypedef char* string
ctypedef dict[string, list[cnp.int_t[:]]] table_spans
ctypedef dict[string, cnp.int_t[:]] table_column
ctypedef cnp.int64_t DTYPE_t

def say_hello_to(name):
    print(f"Hello {name}!")

def project(table_spans table, list[string] col_names):
    return {col_name : table[col_name] for col_name in col_names}

# works on materialised columns
def select(table_spans table_in, list[string] key_col_names, list[string] boolean_ops, list vals):
    cdef table_column table = materialise_into_columns(table_in)

    cdef list[string] col_names = list(table.keys())
    cdef cnp.npy_bool[:] bools = np.full(len(table[col_names[0]]), True)
    cdef int i
    cdef string col_name
    cdef string op
    cdef cnp.int_t[:] npy_arr
    for i in range(len(key_col_names)):
        col_name = key_col_names[i]
        op = boolean_ops[i]
        val = vals[i]
        npy_arr = table[col_name]
        bools &= boolean_op[op](npy_arr, val)
    return {col_name: table[col_name][bools] for col_name in col_names}

# accepts and returns tables /w materialised columns
def equi_join(table_spans table_1_in, table_spans table_2_in, list[string] key_col_names_1, list[string] key_col_names_2):
    cdef table_column table_1 = materialise_into_columns(table_1_in)
    cdef table_column table_2 = materialise_into_columns(table_2_in)

    cdef list[string] col_names_1 = list(table_1.keys())
    cdef list[string] col_names_2 = list(table_2.keys())
    cdef list[string] keys_1 = [table_1[col_name] for col_name in key_col_names_1]
    cdef list[string] keys_2 = [table_2[col_name] for col_name in key_col_names_2]
    cdef cnp.int_t[:] sort_ixs_1 = np.lexsort(keys_1)
    cdef cnp.int_t[:] sort_ixs_2 = np.lexsort(keys_2)
    cdef table_column table_1_sorted = {col_name: table_1[col_name][sort_ixs_1] for col_name in col_names_1}
    cdef table_column table_2_sorted = {col_name: table_2[col_name][sort_ixs_2] for col_name in col_names_2}
    cdef list[int] res_ix_1 = []
    cdef list[int] res_ix_2 = []
    cdef int i = 0
    cdef int j = 0
    cdef int j_start = 0
    cdef bint first
    cdef bint same
    cdef string col_name_1
    cdef string col_name_2
    cdef cnp.int_t elem_1
    cdef cnp.int_t elem_2
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
    
    cdef cnp.int_t[:] res_ix_1_npy = np.array(res_ix_1)
    cdef cnp.int_t[:] res_ix_2_npy = np.array(res_ix_2)
    cdef table_column table_1_joined = {col_name: table_1_sorted[col_name][res_ix_1_npy] for col_name in col_names_1}
    cdef table_column table_2_joined = {col_name: table_2_sorted[col_name][res_ix_2_npy] for col_name in col_names_2}
    cdef table_column res = table_1_joined | table_2_joined
    return res

# accepts and returns tables /w materialised columns
def aggregate(table_spans table_in, list[string] key_col_names, list[string] reduction_func, list[string] reduction_col_name):
    cdef table_column table = materialise_into_columns(table_in)

    cdef list[string] col_names = list(table.keys())
    cdef list[string] key_cols = [table[col_name] for col_name in key_col_names]
    cdef cnp.int_t[:] sort_ixs = np.lexsort(key_cols)
    cdef table_column table_sorted = {col_name: table[col_name][sort_ixs] for col_name in col_names}
    cdef list[int] splits = []
    cdef int i = 0
    cdef int j = 0
    cdef bint same
    cdef string col_name
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
    
    cdef table_spans table_split_up = {
        col_name: [x for x in np.split(table_sorted[col_name], splits) if len(x) > 0] 
        for col_name in col_names
    }
    cdef list reduced_col = [reduction_functions[reduction_func](x) for x in table_split_up[reduction_col_name]]
    cdef table_column table_reduced = {reduction_col_name: np.array(reduced_col)}
    cdef table_column table_key = {col_name: np.array([x[0] for x in table_split_up[col_name]]) for col_name in key_col_names}
    cdef table_column res = table_key | table_reduced
    # print(res)
    return res

cdef table_column materialise_into_columns(table_spans table):
    return {col_name: np.concatenate([table[col_name]]).ravel() for col_name in table.keys()}

cdef split_into_spans(table, span_size):
    col_names = list(table.keys())
    num_splits = math.ceil(len(table[col_names[0]]) / span_size)
    splits = np.array([(i + 1) * span_size for i in range(num_splits)])
    return {col_name: [x for x in np.split(table[col_name], splits) if len(x) > 0] for col_name in col_names}

reduction_functions = {
    'sum': lambda x: np.sum(x),
    'prod': lambda x: np.prod(x),
    'count': lambda x: x.size,
    'avg': lambda x: np.mean(x),
    'max': lambda x: np.max(x),
    'min': lambda x: np.min(x),
}

boolean_op = {
    '==': lambda x, y: np.equal(x, y),
    '!=': lambda x, y: np.not_equal(x, y),
    '<': lambda x, y: np.less(x, y),
    '<=': lambda x, y: np.less_equal(x, y),
    '>': lambda x, y: np.greater(x, y),
    '>=': lambda x, y: np.greater_equal(x, y),
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
    