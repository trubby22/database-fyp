import numpy as np

# bar = lambda a, b: a < b
# def bar(a, b):
#     return a < b
# baz = bar(x)
# res = baz(3)
# print(res)

# def dispatch(arr, op, val):
#     match op:
#         case '<':
#             return arr < val
#         case '>':
#             return arr > val
#         case '==':
#             return arr == val
#         case '!=':
#             return arr != val
#         case '<=':
#             return arr <= val
#         case '>=':
#             return arr >= val
#         case _:
#             raise Exception()

# x = np.array([1, 2, 3])
# ix = [1, 0, 2]
# print(x[ix])
# print(x[np.array(ix)])

# foo = {
#     'bar': lambda x: np.sum(x),
#     'baz': lambda x, y: x < y,
# }

# xs = np.array([1, 2, 3])
# print(xs[foo['baz'](xs, 2)])

# a = np.array([True, True, False])
# b = np.array([True, False, True])
# c = a & b
# print(c)

# print(xs[np.array([0, 0, 0, 1, 1, 1, 2, 2, 2])])

# xs = np.array([1, 2, 3])
# splits = np.array([1])
# [a, b] = np.split(xs, splits)
# print([a, b])

xs = np.tile(np.arange(5, 10, 1), 2)
print(xs)
