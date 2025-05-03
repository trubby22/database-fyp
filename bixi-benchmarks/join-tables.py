import pandas as pd

# Load data from a CSV file
trips = pd.read_csv('./bixi-data/OD_2017.csv')
stations = pd.read_csv('./bixi-data/Stations_2017.csv')

# Display the first few rows of the DataFrame
print(trips.head())
print(stations.head())

start_merged = pd.merge(trips, stations, left_on='start_station_code', right_on='code')
print(start_merged.head())

start_end_merged = pd.merge(start_merged, stations, left_on='end_station_code', right_on='code')
print(start_end_merged.head())

print(start_end_merged.columns)

projected = start_end_merged[['duration_sec', 'latitude_x', 'longitude_x', 'latitude_y', 'longitude_y']]
print(projected)

projected.to_csv('bixi-clean.csv')
