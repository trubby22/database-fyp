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

foo = {
    'bar': lambda x: np.sum(x)
}

xs = np.array([1, 2, 3])
print(foo['bar'](xs))
