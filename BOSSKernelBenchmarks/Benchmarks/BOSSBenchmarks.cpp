#include <BOSS.hpp>
#include <ExpressionUtilities.hpp>

#include "config.hpp"
#include "utilities.cpp"

#include <benchmark/benchmark.h>
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

vector<string> rand_table_names = {
  "_1mb",
  "_10mb",
  "_100mb",
  "_1gb"
};

unordered_map<string, ComplexExpression> tbd_map = {
  {"_1mb", },
  {"_10mb", },
  {"_100mb", },
  {"_1gb", },
};

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
  auto reversedLibraries = librariesToTest;
  std::reverse(reversedLibraries.begin(), reversedLibraries.end());
  boss::expressions::ExpressionSpanArguments spans;
  spans.emplace_back(boss::expressions::Span<std::string>(reversedLibraries));
  boss::evaluate("ReleaseEngines"_(boss::ComplexExpression("List"_, {}, {}, std::move(spans))));
}

ComplexExpression create_rand_table(string table_name) {
  return "CreateTable"_(Symbol(table_name),
  "c1"_, "As"_("DOUBLE"_),
  "c2"_, "As"_("DOUBLE"_),
  "c3"_, "As"_("DOUBLE"_),
  "c4"_, "As"_("DOUBLE"_),
  "c5"_, "As"_("DOUBLE"_),
  "c6"_, "As"_("DOUBLE"_),
  "c7"_, "As"_("DOUBLE"_),
  "c8"_, "As"_("DOUBLE"_),
  );
}

ComplexExpression create_bixi_table() {
  return "CreateTable"_("bixi"_, 
    "duration_sec"_, "As"_("BIGINT"_),
    "latitude_x"_, "As"_("DOUBLE"_),
    "longitude_x"_, "As"_("DOUBLE"_),
    "latitude_y"_, "As"_("DOUBLE"_),
    "longitude_y"_, "As"_("DOUBLE"_)
  );
}

ComplexExpression create_table(string name) {
  if (name == "bixi") {
    return create_bixi_table();
  } else {
    return create_rand_table(name);
  }
}

void initStorageEngine() {
  resetStorageEngine();

  auto evalStorage = getEvaluateStorageLambda();
  auto checkForErrors = getCheckForErrorsLambda();
  auto eval = getEvaluateLambda();

  checkForErrors(eval("Set"_("LoadToMemoryMappedFiles"_, false)));
  if(DEFAULT_STORAGE_BLOCK_SIZE > 0) {
    checkForErrors(evalStorage("Set"_("FileLoadingBlockSize"_, DEFAULT_STORAGE_BLOCK_SIZE)));
  }

  checkForErrors(evalStorage("CreateTable"_("bixi"_, 
  "duration_sec"_, "As"_("BIGINT"_),
  "latitude_x"_, "As"_("DOUBLE"_),
  "longitude_x"_, "As"_("DOUBLE"_),
  "latitude_y"_, "As"_("DOUBLE"_),
  "longitude_y"_, "As"_("DOUBLE"_)
  )));

  for (auto &sf : scaling_factors) {
    ostringstream oss;
    oss << "sf-" << sf;
    string table_name_str = oss.str();
    auto table = create_rand_table(table_name_str);
    checkForErrors(evalStorage(move(table)));
  }

  std::string path = "/root/Documents/4-year/fyp-70011/bixi-data/bixi-no-index-yes-colnames.csv";
  Symbol table = "bixi"_;
  checkForErrors(evalStorage("Load"_(table, path)));

  string path_prefix = "/root/Documents/4-year/fyp-70011/rand-table-data/";
  string path_suffix = ".csv";
  for (auto &sf : scaling_factors) {
    ostringstream path_oss;
    path_oss << << path_prefix << "sf-" << sf << path_suffix;
    string path = path_oss.str();

    ostringstream table_name_oss;
    table_name_oss << "sf-" << sf;
    string table_name = table_name_oss.str();
    Symbol table = Symbol(table_name);
    checkForErrors(evalStorage("Load"_(table, path)));
  }
}

void create_and_load_table(string &name_str, string &&path) {
  Symbol name = Symbol(name_str);
  ComplexExpression create_table_expr = create_table(name_str);

  auto checkForErrors = getCheckForErrorsLambda();
  auto evalStorage = getEvaluateStorageLambda();
  auto eval = getEvaluateLambda();

  checkForErrors(evalStorage(move(create_table_expr)));
  checkForErrors(evalStorage("Load"_(move(name), move(path))));
}

ComplexExpression python_import_numpy() {
  return "Python"_("import numpy as np"_, "Where"_());
}

#pragma region queries

ComplexExpression data_in() {
  return "Python"_(""_, "Where"_("rand_table"_, "rand_table"_))
}

ComplexExpression round_trip() {
  return "And"_(
    "Python"_(""_, "Where"_("rand_table"_, "rand_table"_)),
    "get_python_var"_("rand_table"_)
  )
}

ComplexExpression materialise_columns() {
  return "And"_(
    "Python"_(R"(
table = rand_table['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)
    )", "Where"_("rand_table"_, "rand_table"_)),
    "get_python_var"_("rand_table"_)
  )
}

ComplexExpression materialise_matrix() {
  return "And"_(
    "Python"_(R"(
table = rand_table['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)

m = np.stack(table_cpy.values(), axis=0) # matrix row = table column
    )", "Where"_("rand_table"_, "rand_table"_)),
    "get_python_var"_("rand_table"_)
  )
}

// weighted sum
ComplexExpression matrix_vector_product() {
  return "And"_(
    "Python"_(R"(
table = rand_table['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)

m = np.stack(table_cpy.values(), axis=0) # matrix row = table column
w = np.array(
  [8.41, 3.14, 5.29, -3.81, 0.03, -6.42, -8.37, 2.78], 
  dtype=np.float64)
res = m @ w

    )", "Where"_("rand_table"_, "rand_table"_)),
    "get_python_var"_("rand_table"_)
  )
}

ComplexExpression matrix_matrix_product() {
  return "And"_(
    "Python"_(R"(
table = rand_table['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)

m = np.stack(table_cpy.values(), axis=0) # matrix row = table column
res = m @ m.T

    )", "Where"_("rand_table"_, "rand_table"_)),
    "get_python_var"_("rand_table"_)
  )
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

for i in range(500):
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
                "bixi"_, "bixi"_
              )
          );
}

#pragma endregion queries

table_paths = unordered_map<> {
  {"table-to-load-name", "table-path"}
};

random_tables = vector<string> {
  {"csv-name", "table-to-load-name"}
};

void initAndRunBenchmarks() {
  init_libraries();
  storageLibrary = USING_COORDINATOR_ENGINE ? librariesToTest[1] : librariesToTest[0];
  initStorageEngine();
  auto eval = getEvaluateLambda();
  eval(python_import_numpy());
  ostringstream csv;
  csv << "sf,data-in,round-trip,materialise-columns,materialise-matrix,matrix-vector-product,matrix-matrix-product" << endl;

  for (string sf : scaling_factors) {
    ostringstream table_name_oss;
    table_name_oss << "sf-" << sf;
    string table_name = oss.str();

    initTable(table);

    csv << table_name;

    for (auto query : random_queries) {

      for (int i = 0; i < num_warmup; i++) {
        eval(query);
      }

      chrono start;
      for (int i = 0; i < num_main; i++) {
        auto res = eval(query);
        benchmark::DoNotOptimize(res);
      }
      chrono end;
      chrono avg_nano = (end - start) / num_main;

      csv << "," << avg_nano;
    }
    csv << endl;

    resetStorageEngine();
  }

  string csv_str = csv.str();
  csv_str.to_file(path/to/csv);

  csv.str("");
  csv << "data-in,processing" << endl;

  auto table = bixi_table;
  initTable(table);

  bixi_query();

  resetStorageEngine();


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
