#include <BOSS.hpp>
#include <ExpressionUtilities.hpp>

#include "config.hpp"
#include "utilities.cpp"
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <chrono>

using namespace boss::utilities;
using namespace std;
using intType = int32_t;
using string_literals::operator"" s;
using boss::utilities::operator""_;
using boss::ComplexExpression;
using boss::Expression;
using boss::Span;
using boss::Symbol;
using boss::expressions::ComplexExpressionWithStaticArguments;
using boss::expressions::ExpressionArguments;
using boss::expressions::ExpressionSpanArgument;
using boss::expressions::ExpressionSpanArguments;
using boss::expressions::CloneReason;

bool USING_COORDINATOR_ENGINE = false;
bool VERBOSE_QUERY_OUTPUT = false;
bool VERY_VERBOSE_QUERY_OUTPUT = false;
bool VERIFY_QUERY_OUTPUT = false;
bool ENABLE_CONSTRAINTS = false;
int BENCHMARK_MIN_WARMPUP_ITERATIONS = 3;
int BENCHMARK_MIN_WARMPUP_TIME = 1;
uint64_t SPAN_SIZE_FOR_NON_ARROW = 1U << 23U;
bool BENCHMARK_STORAGE_BLOCK_SIZE = false;
int64_t DEFAULT_STORAGE_BLOCK_SIZE = 0; // 0: keep storage's default
int VELOX_INTERNAL_BATCH_SIZE = 0;
int VELOX_MINIMUM_OUTPUT_BATCH_SIZE = 0;

std::vector<std::string> librariesToTest = {};
std::string storageLibrary = {};
int latestDataSize = -1;
int latestBlockSize = -1;
std::string latestDataSet;

void print_elapsed_time(chrono::nanoseconds elapsed_time) {
  cout << "elapsed time = " << chrono::duration_cast<chrono::seconds>(elapsed_time).count() << "[s]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::milliseconds>(elapsed_time).count() << "[ms]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::microseconds>(elapsed_time).count() << "[µs]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::nanoseconds> (elapsed_time).count() << "[ns]" << endl;
}

void init_libraries() {
  librariesToTest.emplace_back("/mnt/ubuntu-image-repos/BOSSArrowStorageEngine/build/libBOSSArrowStorage.so");
  librariesToTest.emplace_back("/mnt/ubuntu-image-repos/BOSSNumpyEngine/build/libBOSSNumpyEngine.so");
}

static void releaseBOSSEngines() {
  // make sure to release engines in reverse order of evaluation
  // (important for data ownership across engines)
  auto reversedLibraries = librariesToTest;
  std::reverse(reversedLibraries.begin(), reversedLibraries.end());
  boss::expressions::ExpressionSpanArguments spans;
  spans.emplace_back(boss::expressions::Span<std::string>(reversedLibraries));
  boss::evaluate("ReleaseEngines"_(boss::ComplexExpression("List"_, {}, {}, std::move(spans))));
}

void initStorageEngine_bixi() {
  resetStorageEngine();


  auto evalStorage = getEvaluateStorageLambda();
  auto checkForErrors = getCheckForErrorsLambda();
  auto eval = getEvaluateLambda();

  checkForErrors(eval("Set"_("LoadToMemoryMappedFiles"_, false)));
  if(DEFAULT_STORAGE_BLOCK_SIZE > 0) {
    checkForErrors(evalStorage("Set"_("FileLoadingBlockSize"_, DEFAULT_STORAGE_BLOCK_SIZE)));
  }

  checkForErrors(evalStorage("CreateTable"_("BIXI"_, 
  "duration_sec"_, "As"_("BIGINT"_),
  "latitude_x"_, "As"_("DOUBLE"_),
  "longitude_x"_, "As"_("DOUBLE"_),
  "latitude_y"_, "As"_("DOUBLE"_),
  "longitude_y"_, "As"_("DOUBLE"_)
  )));

  auto filenamesAndTables = std::vector<std::pair<std::string, boss::Symbol>>{
      {"bixi-clean", "BIXI"_}};

  for(auto const& [filename, table] : filenamesAndTables) {
    std::string path =
        tpch_filePath_prefix + "bixi-data/" + filename + ".tbl";
    checkForErrors(evalStorage("Load"_(table, path)));
  }
}

ComplexExpression bixi_query() {
  return "Python"_(
    R"(
import numpy as np

print(bixi)
table = bixi['table']

dur = table['duration_sec']
lon_x = table['longitude_x']
lat_x = table['latitude_x']
lon_y = table['longitude_y']
lat_y = table['latitude_y']

dur = np.concatenate(dur)
lon_x = np.concatenate(lon_x)
lat_x = np.concatenate(lat_x)
lon_y = np.concatenate(lon_y)
lat_y = np.concatenate(lat_y)

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
print(dist)

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
pred = np.reshape(pred, -1)

def squared_err(act, pred):
    errors = np.square(pred - act)
    sum_err = np.sum(errors)
    num_vals = act.shape[0]
    res = sum_err / (2 * num_vals)
    return res

sq_err = squared_err(train_out, pred)
print(sq_err)

def grad_desc(act, pred, indata):
    return (pred - act).T @ indata / act.shape[0]

alpha = 0.1

params = params - alpha * grad_desc(train_out, pred, train_in)
print(params)

pred = train_in @ params.T
pred = np.reshape(pred, -1)
sq_err = squared_err(train_out, pred)
print(sq_err)

for i in range(100):
    pred = train_in @ params.T
    pred = np.reshape(pred, -1)
    params = params - alpha * grad_desc(train_out, pred, train_in)
    sq_err = squared_err(train_out, pred)
    
    if( (i+1) % 100 == 0):
        print(f"Error rate after {i + 1} iterations is {sq_err}")
    
print(params)
sq_err = squared_err(train_out, pred)
print(sq_err)

test_pred = test_in @ params.T
test_pred = np.reshape(test_pred, -1)

sq_err = squared_err(test_out * max_dur, test_pred * max_dur)
print(sq_err)
)"_,
    "Where"_(
                "bixi"_, "BIXI"_
              )
          );
}

void initAndRunBenchmarks() {
  init_libraries();
  storageLibrary = USING_COORDINATOR_ENGINE ? librariesToTest[1] : librariesToTest[0];

  initStorageEngine_bixi();
  auto eval = getEvaluateLambda();
  Expression query = bixi_query();

  chrono::high_resolution_clock::time_point begin = chrono::high_resolution_clock::now();

  auto result = eval(move(query));

  chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
  chrono::nanoseconds elapsed_time = chrono::duration_cast<chrono::nanoseconds>(end - begin);
  cout << "bixi" << endl;
  print_elapsed_time(move(elapsed_time));
  cout << endl;

  releaseBOSSEngines();
}

int main(int argc, char** argv) {
  try {
    initAndRunBenchmarks();
  } catch(std::exception& e) {
    std::cerr << "caught exception in main: " << e.what() << std::endl;
    boss::evaluate("ResetEngines"_());
    return EXIT_FAILURE;
  } catch(...) {
    std::cerr << "unhandled exception." << std::endl;
    boss::evaluate("ResetEngines"_());
    return EXIT_FAILURE;
  }
  boss::evaluate("ResetEngines"_());
  return EXIT_SUCCESS;
}
