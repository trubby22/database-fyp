import numpy as np

size = 2 << 20
a = np.random.randint(0, 100, size=size)
b = np.random.randint(0, 100, size=size)
c = np.concatenate([a, b])

print(size)
print(c.shape)
