import sys
sys.path.append("/mnt/ubuntu-image-repos/BOSSNumpyEngine/rel_alg_cython")
import numpy as np
from rel_alg_cython_untyped import *

# unit tests
if __name__ == '__main__':
    # table_1 = {
    #     'col1': np.array([1, 2, 3]),
    #     'col2': np.array([0.8, 3.14, 2.42]),
    #     'col3': np.array([0, 0, 1]),
    # }
    # table_1 = materialise_into_columns(table_1)
    # project_res = project(table_1, ['col2', 'col3'])
    # project_res = split_into_spans(project_res, 10)
    # project_expected = {'col2': np.array([0.8 , 3.14, 2.42]), 'col3': np.array([0, 0, 1])}
    # print('project_res')
    # print(project_res)
    # print('project_expected')
    # print(project_expected)
    # print()

    table_1 = {
        'col1': np.array([1, 2, 3]),
        'col2': np.array([0.8, 3.14, 2.42]),
        'col3': np.array([0, 0, 1]),
    }
    project_res = project(
        table_1,
        ['col1'], ['prod'], ['col1_prod'],
        ['col2'], ['col3'], ['+'], ['col2_plus_col3'],
        ['col1_prod', 'col2_plus_col3'],
        ['col1_prod', 'col2_plus_col3'],
    )
    print('table_1')
    print(table_1)
    print('project_res')
    print(project_res)
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
        'col1': [np.array([0, 1, 0, 1, 0, 1, 0, 1])],
        'col2': [np.array([0, 0, 1, 1, 0, 0, 1, 1])],
        'col3': [np.array([1, 2, 3, 4, 1, 2, 3, 4])],
    }
    aggregate_res = aggregate(table_4, ['col1', 'col2'], 'sum', 'col3')
    aggregate_expected = {'col1': np.array([0, 1, 0, 1]), 'col2': np.array([0, 0, 1, 1]), 'col3': np.array([2, 4, 6, 8])}
    print('aggregate_res')
    print(aggregate_res)
    print('aggregate_expected')
    print(aggregate_expected)
    print()

