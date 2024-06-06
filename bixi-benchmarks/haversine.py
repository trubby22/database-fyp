import math

def haversine(lat1, lon1, lat2, lon2):
    # Convert latitude and longitude from degrees to radians
    lat1, lon1, lat2, lon2 = map(math.radians, [lat1, lon1, lat2, lon2])
    
    # Haversine formula
    dlat = lat2 - lat1
    dlon = lon2 - lon1
    a = math.sin(dlat / 2)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2)**2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    
    # Radius of Earth in kilometers. Use 6371 for kilometers or 3956 for miles
    r = 6371
    
    # Calculate the result in meters
    distance = r * c * 1000
    return distance

# Example usage:
lat1 = 52.2296756
lon1 = 21.0122287
lat2 = 41.8919300
lon2 = 12.5113300

distance = haversine(lat1, lon1, lat2, lon2)
print(f"The distance between the points is {distance} meters.")
