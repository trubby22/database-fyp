#pragma region includes

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
#include <fstream>
#include <map>
#include <unordered_map>

#pragma endregion includes

// #define DEBUG

#pragma region usings

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
using utilities::shallowCopy;

#pragma endregion usings

typedef unsigned long long ull;

#pragma region globals

const string rand_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/boss-rand-results.csv";
const string bixi_results_path = "/root/Documents/4-year/fyp-70011/experiment-results/boss-bixi-results.csv";

std::vector<std::string> librariesToTest = {};
std::string storageLibrary = {};

map<string, string> rand_names_paths = {
  // {"_1_64b", "/root/Documents/4-year/fyp-70011/data/csv/_64b.csv"},
  {"_2_1mb", "/root/Documents/4-year/fyp-70011/data/csv/_1mb.csv"},
  // {"_3_10mb", "/root/Documents/4-year/fyp-70011/data/csv/_10mb.csv"},
  // {"_4_100mb", "/root/Documents/4-year/fyp-70011/data/csv/_100mb.csv"},
  // {"_5_1gb", "/root/Documents/4-year/fyp-70011/data/csv/_1gb.csv"},
  // {"_6_2gb", "/root/Documents/4-year/fyp-70011/data/csv/_2gb.csv"},
};

unordered_map<string, string> rand_names = {
  {"_1_64b", "64 b"},
  {"_2_1mb", "1 mb"},
  {"_3_10mb", "10 mb"},
  {"_4_100mb", "100 mb"},
  {"_5_1gb", "1 gb"},
  {"_6_2gb", "2 gb"},
};

map<string, string> bixi_names_paths = {
  {"bixi", "/root/Documents/4-year/fyp-70011/data/csv/bixi.csv"}
};

unordered_map<string, string> bixi_names = {
  {"bixi", "bixi"},
};

#pragma endregion globals

#pragma region boilerplate

void init_libraries() {
  librariesToTest.emplace_back("/mnt/ubuntu-image-repos/BOSSArrowStorageEngine/build/libBOSSArrowStorage.so");
  librariesToTest.emplace_back("/mnt/ubuntu-image-repos/BOSSNumpyEngine/build/libBOSSNumpyEngine.so");
}

static void release_boss_engines() {
  auto reversedLibraries = librariesToTest;
  std::reverse(reversedLibraries.begin(), reversedLibraries.end());
  boss::expressions::ExpressionSpanArguments spans;
  spans.emplace_back(boss::expressions::Span<std::string>(reversedLibraries));
  boss::evaluate("ReleaseEngines"_(boss::ComplexExpression("List"_, {}, {}, std::move(spans))));
}

#pragma endregion boilerplate

#pragma region print

void print_elapsed_time(chrono::nanoseconds elapsed_time) {
  cout << "elapsed time = " << chrono::duration_cast<chrono::seconds>(elapsed_time).count() << "[s]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::milliseconds>(elapsed_time).count() << "[ms]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::microseconds>(elapsed_time).count() << "[µs]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::nanoseconds> (elapsed_time).count() << "[ns]" << endl;
}

#pragma endregion print

#pragma region loading

ComplexExpression create_rand_table_expr() {
  return "CreateTable"_("rand_table_boss"_,
    "c1"_, "As"_("DOUBLE"_),
    "c2"_, "As"_("DOUBLE"_),
    "c3"_, "As"_("DOUBLE"_),
    "c4"_, "As"_("DOUBLE"_),
    "c5"_, "As"_("DOUBLE"_),
    "c6"_, "As"_("DOUBLE"_),
    "c7"_, "As"_("DOUBLE"_),
    "c8"_, "As"_("DOUBLE"_)
  );
}

ComplexExpression create_bixi_table_expr() {
  return "CreateTable"_("bixi_boss"_, 
    "duration_sec"_, "As"_("BIGINT"_),
    "latitude_x"_, "As"_("DOUBLE"_),
    "longitude_x"_, "As"_("DOUBLE"_),
    "latitude_y"_, "As"_("DOUBLE"_),
    "longitude_y"_, "As"_("DOUBLE"_)
  );
}

ComplexExpression create_table_expr(string name) {
  if (name == "bixi") {
    return create_bixi_table_expr();
  } else {
    return create_rand_table_expr();
  }
}

void create_and_load_table(string name_str, string path) {
  Symbol name = Symbol(name_str);
  ComplexExpression create_table = create_table_expr(name_str);

  auto checkForErrors = getCheckForErrorsLambda();
  auto evalStorage = getEvaluateStorageLambda();
  auto eval = getEvaluateLambda();

  checkForErrors(evalStorage(move(create_table)));

  if (name_str == "bixi") {
    checkForErrors(evalStorage("Load"_("bixi_boss"_, path)));
  } else {
    checkForErrors(evalStorage("Load"_("rand_table_boss"_, path)));
  }
}

void init_storage_engine() {
  auto checkForErrors = getCheckForErrorsLambda();
  auto eval = getEvaluateLambda();

  unload_all_tables();
  checkForErrors(eval("Set"_("LoadToMemoryMappedFiles"_, false)));
}

#pragma endregion loading

#pragma region queries

ComplexExpression python_import_numpy() {
  return "Python_globals"_("import numpy as np"_);
}

auto& rand_queries() {
  static map<string, ComplexExpression> queries;
  if(queries.empty()) {
//     queries.try_emplace(
//       "_1_data_in", 
//       "Python"_(""_, "Where"_("rand_table_python"_, "rand_table_boss"_))
//     );
    // queries.try_emplace(
    //   "_2_round_trip", 
    //   "And"_(
    //     "Python"_(""_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
    //     "get_python_var"_("rand_table_python"_)
    //   )
    // );
//     queries.try_emplace(
//       "_3_materialise_columns", 
//         "And"_(
//           "Python"_(R"(
// table = rand_table_python['table']
// table_cpy = dict()
// for k in table.keys():
//   spans = table[k]
//   table_cpy[k] = [np.concatenate(spans)]

// # print('table_cpy', table_cpy, sep='\n')
// res_table_python = {'table': table_cpy, 'matrix': None}
//           )"_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
//           "get_python_var"_("rand_table_python"_),
//           "get_python_var"_("res_table_python"_)
//         )
//     );
    queries.try_emplace(
      "_4_materialise_matrix", 
      "And"_(
        "Python"_(R"(
table = rand_table_python['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)

m = np.stack(list(table_cpy.values()), axis=0) # matrix row = table column
# print('m', m, sep='\n')

m_wrapper = {'data': m, 'col_names': list(table.keys())}
res_table_python = {'table': None, 'matrix': m_wrapper}
        )"_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
        "get_python_var"_("rand_table_python"_),
        "get_python_var"_("res_table_python"_)
      )
    );
//     queries.try_emplace(
//       "_5_matrix_vector_product", 
//       "And"_(
//         "Python"_(R"(
// table = rand_table_python['table']
// table_cpy = dict()
// for k in table.keys():
//   spans = table[k]
//   table_cpy[k] = np.concatenate(spans)

// m = np.stack(list(table_cpy.values()), axis=0) # matrix row = table column
// w = np.array(
//   [8.41, 3.14, 5.29, -3.81, 0.03, -6.42, -8.37, 2.78], 
//   dtype=np.float64).reshape((8, 1))
// res = w.T @ m
// #print('m.shape', m.shape)
// #print('res.shape', res.shape)
// #print('res', res, sep='\n')

// res_wrapper = {'data': res, 'col_names': ['aggregate_value']}
// res_table_python = {'table': None, 'matrix': res_wrapper}
//         )"_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
//         "get_python_var"_("rand_table_python"_),
//         "get_python_var"_("res_table_python"_)
//       )
//     );
//     queries.try_emplace(
//       "_6_matrix_matrix_product", 
//       "And"_(
//         "Python"_(R"(
// table = rand_table_python['table']
// table_cpy = dict()
// for k in table.keys():
//   spans = table[k]
//   table_cpy[k] = np.concatenate(spans)

// m = np.stack(list(table_cpy.values()), axis=0) # matrix row = table column
// res = m @ m.T
// # print('res', res, sep='\n')

// res_wrapper = {'data': res, 'col_names': list(table.keys())}
// res_table_python = {'table': None, 'matrix': res_wrapper}
//         )"_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
//         "get_python_var"_("rand_table_python"_),
//         "get_python_var"_("res_table_python"_)
//       )
//     );
  }
  return queries;
}

auto& bixi_queries() {
  static map<string, ComplexExpression> queries;
  if(queries.empty()) {
    queries.try_emplace(
      "_1_data_in",
      "Python"_(
        R"(
#print(bixi_python)
    )"_,
        "Where"_(
                    "bixi_python"_, "bixi_boss"_
                  )
              )
    );
    queries.try_emplace(
      "_2_predict_duration_from_distance",
      "Python"_(
        R"(
#print(bixi_python)
table = bixi_python['table']

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
    )"_,
        "Where"_(
                    "bixi_python"_, "bixi_boss"_
                  )
              )
    );
  }
  return queries;
}

#pragma endregion queries

void benchmark_loop(
  ostringstream &&csv, 
  string csv_path, 
  map<string, string> &table_names_paths,
  unordered_map<string, string> &table_names,
  map<string, ComplexExpression> &query_names_exprs
) {
  auto eval = getEvaluateLambda();
  auto eval_numpy = getEvaluateBaselineLambda();
  cout << endl;

  for (const auto& [table_name, table_path] : table_names_paths) {
    create_and_load_table(table_name, table_path);

    csv << table_names[table_name];

    for (const auto& [query_name, query_expr] : query_names_exprs) {
      cout << "========== start " << table_name << " " << query_name << " ==========" << endl;

      eval_numpy("reset_python_dict"_);

      const chrono::seconds time_warmup = 0s;
      const ull warmup_iters = 0;
      // const chrono::seconds time_warmup = 0s;
      // const int warmup_iters = 0;
      chrono::high_resolution_clock::time_point warmup_start = chrono::high_resolution_clock::now();
      chrono::high_resolution_clock::time_point warmup_end_time = warmup_start + time_warmup;
      chrono::high_resolution_clock::time_point warmup_timestamp = warmup_start;
      for (ull i = 0; i < warmup_iters || warmup_timestamp < warmup_end_time; i++) {
        auto res = eval(shallowCopy(query_expr));
        benchmark::DoNotOptimize(res);
        warmup_timestamp = chrono::high_resolution_clock::now();
      }

      const chrono::seconds time_test = 0s;
      const ull test_iters = 1;
      // const chrono::seconds time_test = 0s;
      // const int test_iters = 1;
      chrono::high_resolution_clock::time_point test_start = chrono::high_resolution_clock::now();
      chrono::high_resolution_clock::time_point test_end_time = test_start + time_test;
      chrono::high_resolution_clock::time_point test_timestamp = test_start;
      ull completed_iters = 0;
      for (completed_iters = 0; completed_iters < test_iters || test_timestamp < test_end_time; completed_iters++) {
        auto res = eval(shallowCopy(query_expr));
        benchmark::DoNotOptimize(res);
        test_timestamp = chrono::high_resolution_clock::now();
#ifdef DEBUG
        cout << "res" << endl;
        cout << res << endl;
        cout << endl;
#endif
      }

      chrono::high_resolution_clock::time_point test_end = chrono::high_resolution_clock::now();
      chrono::nanoseconds elapsed_time = chrono::duration_cast<chrono::nanoseconds>(test_end - test_start);
      chrono::nanoseconds avg_time = elapsed_time / completed_iters;

#ifdef DEBUG
      cout << query_expr << endl;
#endif
      print_elapsed_time(avg_time);
      cout << endl;
      cout << "end " << table_name << " " << query_name << endl;
      cout << endl;

      csv << "," << avg_time.count();
    }
    csv << endl;

    unload_all_tables();
  }

  string csv_str = csv.str();
  ofstream outfile(csv_path);
  outfile << csv_str;
  outfile.close();
}

void init_and_run_benchmarks() {
  init_libraries();
  storageLibrary = librariesToTest[0];
  init_storage_engine();
  auto eval = getEvaluateLambda();
  eval(python_import_numpy());

  ostringstream csv;
  csv << "table name,boss data in,boss round trip,boss materialise columns,boss materialise matrix,boss matrix vector product,boss matrix matrix product" << endl;  
  benchmark_loop(
    move(csv),
    rand_results_path,
    rand_names_paths,
    rand_names,
    rand_queries()
  );

  // csv.str("");
  // csv << "table name,boss data in,boss predict duration from distance" << endl;
  // benchmark_loop(
  //   move(csv),
  //   bixi_results_path,
  //   bixi_names_paths,
  //   bixi_names,
  //   bixi_queries()
  // );

  unload_all_tables();
  release_boss_engines();
}

int main() {
  try {
    init_and_run_benchmarks();
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
