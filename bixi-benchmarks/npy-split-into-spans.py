import numpy as np

arr = np.arange(10)
# arr1 = arr[:2]
# arr2 = arr[2:]

# print(arr1)
# print(arr2)

def split_into_spans(npy_arr, span_size):
    n = len(npy_arr)
    res = []
    i = 0
    while i < n:
        cur = npy_arr[i : i + span_size]
        res.append(cur)
        i += span_size
    return res

res = split_into_spans(arr, 3)
print(res)

col = np.concatenate(res)
print(col)

