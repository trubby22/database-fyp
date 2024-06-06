import pandas as pd

# Sample DataFrame
data = {
    'A': [1, 2, 3, 4],
    'B': [5, 6, 7, 8],
    'C': [9, 10, 11, 12]
}
df = pd.DataFrame(data)

# Extract column 'B' and convert to a NumPy array
column_b = df['B'].to_numpy()

# Print the NumPy array
print(type(column_b))
print(column_b.dtype)
