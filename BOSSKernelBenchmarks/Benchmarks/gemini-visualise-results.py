import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load data from CSV file
data = pd.read_csv("/root/Documents/4-year/fyp-70011/experiment-results/results.csv")

# Extract scaling factors and task names
scaling_factors = data.iloc[:, 0].str.strip("_")  # Remove underscores
task_names = data.columns[1:]

# Prepare colors for different tasks (adjust as needed)
colors = ["red", "green", "blue", "purple", "orange", "cyan", "magenta"]

# Create the bar chart
fig, ax = plt.subplots(figsize=(10, 6))  # Adjust figure size as needed
width = 0.35  # Adjust bar width as needed

# Iterate over scaling factors and plot bars
for i, scaling_factor in enumerate(scaling_factors):
  start_pos = i * width
  ax.bar(
      start_pos + np.arange(len(task_names)) / (len(scaling_factors) + 1),
      data.iloc[i, 1:],
      width,
      label=scaling_factor,
      color=colors[i % len(colors)],  # Cycle through colors
  )

# Set labels and title
ax.set_xlabel("Task")
ax.set_ylabel("Time (ns)")
ax.set_title("Benchmarking Results")

# Add legend
ax.legend()

# Rotate x-axis labels for readability (optional)
plt.xticks(rotation=45, ha="right")

# Show the chart
plt.tight_layout()
plt.show()
