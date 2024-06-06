from time import time_ns

start = time_ns()
for i in range(1 << 26):
    pass
stop = time_ns()
delta = stop - start
print(delta / (10 ** 6))

