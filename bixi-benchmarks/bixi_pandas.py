import numpy as np
import pandas as pd

def go(trips: pd.DataFrame):
    #print(trips)
    #print(trips.dtypes)

    table = dict()
    table['duration_sec'] = trips['duration_sec'].to_numpy()
    table['longitude_x'] = trips['longitude_x'].to_numpy()
    table['latitude_x'] = trips['latitude_x'].to_numpy()
    table['longitude_y'] = trips['longitude_y'].to_numpy()
    table['latitude_y'] = trips['latitude_y'].to_numpy()
    bixi = {'table': table, 'matrix': None}
    #print(bixi)
    dur = table['duration_sec']
    lon_x = table['longitude_x']
    lat_x = table['latitude_x']
    lon_y = table['longitude_y']
    lat_y = table['latitude_y']

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

    table['distance'] = haversine_distance(lon_x, lat_x, lon_y, lat_y)
    dist = table['distance']
    #print(dist)

    shuffle_ixs = np.random.permutation(len(dist))
    dist = dist[shuffle_ixs]
    dur = dur[shuffle_ixs]

    max_dist = np.max(dist)
    max_dur = np.max(dur)

    dist = dist / max_dist
    dur = dur / max_dur

    train_ratio = 0.7
    test_ratio = 0.3
    split_ix = int(len(dist) * 0.7)

    dist_train = dist[ : split_ix]
    dur_train = dur[ : split_ix]

    dist_test = dist[split_ix : ]
    dur_test = dur[split_ix : ]

    ones_train = np.ones((len(dist_train),))
    train_in = np.stack((ones_train, dist_train), axis=-1)
    train_out = dur_train
    params = np.ones((1, 2))

    ones_test = np.ones((len(dist_test),))
    test_in = np.stack((ones_test, dist_test), axis=-1)
    test_out = dur_test

    pred = train_in @ params.T
    print('train_in', train_in, sep='\n')
    print('params.T', params.T, sep='\n')
    print('pred', pred, sep='\n')

    pred = np.reshape(pred, -1)

    def squared_err(act, pred):
        errors = np.square(pred - act)
        sum_err = np.sum(errors)
        num_vals = act.shape[0]
        res = sum_err / (2 * num_vals)
        return res

    sq_err = squared_err(train_out, pred)
    #print(sq_err)

    def grad_desc(act, pred, indata):
        return (pred - act).T @ indata / act.shape[0]

    alpha = 0.1

    params = params - alpha * grad_desc(train_out, pred, train_in)
    #print(params)

    pred = train_in @ params.T
    pred = np.reshape(pred, -1)
    sq_err = squared_err(train_out, pred)
    #print(sq_err)

    for i in range(50):
        pred = train_in @ params.T
        pred = np.reshape(pred, -1)
        params = params - alpha * grad_desc(train_out, pred, train_in)
        sq_err = squared_err(train_out, pred)
        
        #if( (i+1) % 100 == 0):
            #print(f"Error rate after {i + 1} iterations is {sq_err}")
        
    #print(params)
    sq_err = squared_err(train_out, pred)
    #print(sq_err)

    test_pred = test_in @ params.T
    test_pred = np.reshape(test_pred, -1)

    sq_err = squared_err(test_out * max_dur, test_pred * max_dur)
    #print(sq_err)
