import numpy as np

def haversine_distance(longitude_x, latitude_x, longitude_y, latitude_y):
    # Convert latitude and longitude from degrees to radians
    longitude_x = np.radians(longitude_x)
    latitude_x = np.radians(latitude_x)
    longitude_y = np.radians(longitude_y)
    latitude_y = np.radians(latitude_y)
    
    # Haversine formula
    dlon = longitude_y - longitude_x
    dlat = latitude_y - latitude_x
    a = np.sin(dlat / 2)**2 + np.cos(latitude_x) * np.cos(latitude_y) * np.sin(dlon / 2)**2
    c = 2 * np.arcsin(np.sqrt(a))
    
    # Radius of Earth in kilometers (mean radius)
    r = 6371.0
    
    # Distance in kilometers
    distance_km = r * c
    
    # Convert distance to meters
    distance_m = distance_km * 1000
    
    return distance_m

# Example usage
longitude_x = np.array([0, 1])
latitude_x = np.array([0, 1])
longitude_y = np.array([1, 2])
latitude_y = np.array([1, 2])

distances = haversine_distance(longitude_x, latitude_x, longitude_y, latitude_y)
print(distances)