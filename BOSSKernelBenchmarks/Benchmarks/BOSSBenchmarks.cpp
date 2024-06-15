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
#include <unordered_set>

#pragma endregion includes

#define DEBUG

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
string query_to_run = {};
string input_size_mb = {};

map<string, string> rand_names_paths = {
  // {"_1_64b", "/mnt/data/csv/_64b.csv"},
  // {"_2_1mb", "/mnt/data/csv/_1mb.csv"},
  // {"_3_10mb", "/mnt/data/csv/_10mb.csv"},
  // {"_4_100mb", "/mnt/data/csv/_100mb.csv"},
  // {"_5_1gb", "/mnt/data/csv/_1gb.csv"},
  {"_6_2gb", "/mnt/data/csv/_2gb.csv"},
};

unordered_map<string, string> rand_names = {
  // {"_1_64b", "64 b"},
  // {"_2_1mb", "1 mb"},
  // {"_3_10mb", "10 mb"},
  // {"_4_100mb", "100 mb"},
  // {"_5_1gb", "1 gb"},
  {"_6_2gb", "2 gb"},
};

map<string, string> bixi_names_paths = {
  {"bixi", "/mnt/data/csv/bixi.csv"}
};

unordered_map<string, string> bixi_names = {
  {"bixi", "bixi"},
};

#pragma endregion globals

#pragma region foo

// Base case for variadic template recursion
template <typename U>
void addToVector(__attribute__((unused)) std::vector<U>& vec) {}

// Recursive variadic template function
template <typename T, typename... Args>
void addToVector(std::vector<T>& vec, const T& first, const Args&... args) {
    vec.push_back(first);
    addToVector<T>(vec, args...);
}

// Function that takes a variable number of arguments
template <typename T, typename... Args>
std::vector<T> makeVector(const Args&... args) {
    std::vector<T> vec;
    addToVector<T>(vec, args...);
    return vec;
}

// Function that takes a variable number of arguments
template <typename T, typename... Args>
boss::Span<T> makeSpan(const Args&... args) {
    return boss::Span<T>{makeVector<T>(args...)};
}

// Function that takes a variable number of arguments
template <typename T, typename... Args>
ComplexExpression makeList(const Args&... args) {
    return "List"_(makeSpan<T>(args...));
}

template <typename... Args>
ComplexExpression int_list(const Args&... args) {
    return makeList<int32_t>(args...);
}

template <typename... Args>
ComplexExpression double_list(const Args&... args) {
    return makeList<double>(args...);
}

template <typename... Args>
ComplexExpression string_list(const Args&... args) {
    return makeList<std::string>(args...);
}

#pragma endregion foo

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
  checkForErrors(eval("Set"_("LoadToMemoryMappedFiles"_, true)));
}

void initStorageEngine_TPCH() {


  auto evalStorage = getEvaluateStorageLambda();
  auto checkForErrors = getCheckForErrorsLambda();

  checkForErrors(evalStorage("Set"_("LoadToMemoryMappedFiles"_, true)));

  checkForErrors(evalStorage("CreateTable"_(
      "LINEITEM"_, "l_orderkey"_, "l_partkey"_, "l_suppkey"_, "l_linenumber"_, "l_quantity"_,
      "l_extendedprice"_, "l_discount"_, "l_tax"_, "l_returnflag"_, "l_linestatus"_, "l_shipdate"_,
      "l_commitdate"_, "l_receiptdate"_, "l_shipinstruct"_, "l_shipmode"_, "l_comment"_)));

  checkForErrors(evalStorage("CreateTable"_("REGION"_, "r_regionkey"_, "r_name"_, "r_comment"_)));

  checkForErrors(evalStorage(
      "CreateTable"_("NATION"_, "n_nationkey"_, "n_name"_, "n_regionkey"_, "n_comment"_)));

  checkForErrors(
      evalStorage("CreateTable"_("PART"_, "p_partkey"_, "p_name"_, "p_mfgr"_, "p_brand"_, "p_type"_,
                                 "p_size"_, "p_container"_, "p_retailprice"_, "p_comment"_)));

  checkForErrors(
      evalStorage("CreateTable"_("SUPPLIER"_, "s_suppkey"_, "s_name"_, "s_address"_, "s_nationkey"_,
                                 "s_phone"_, "s_acctbal"_, "s_comment"_)));

  checkForErrors(evalStorage("CreateTable"_("PARTSUPP"_, "ps_partkey"_, "ps_suppkey"_,
                                            "ps_availqty"_, "ps_supplycost"_, "ps_comment"_)));

  checkForErrors(
      evalStorage("CreateTable"_("CUSTOMER"_, "c_custkey"_, "c_name"_, "c_address"_, "c_nationkey"_,
                                 "c_phone"_, "c_acctbal"_, "c_mktsegment"_, "c_comment"_)));

  checkForErrors(evalStorage("CreateTable"_(
      "ORDERS"_, "o_orderkey"_, "o_custkey"_, "o_orderstatus"_, "o_totalprice"_, "o_orderdate"_,
      "o_orderpriority"_, "o_clerk"_, "o_shippriority"_, "o_comment"_)));

  auto filenamesAndTables = std::vector<std::pair<std::string, boss::Symbol>>{
      {"lineitem", "LINEITEM"_}, {"region", "REGION"_},     {"nation", "NATION"_},
      {"part", "PART"_},         {"supplier", "SUPPLIER"_}, {"partsupp", "PARTSUPP"_},
      {"customer", "CUSTOMER"_}, {"orders", "ORDERS"_}};

  for(auto const& [filename, table] : filenamesAndTables) {
    std::string path =
        "/mnt/ubuntu-image-repos/BOSSKernelBenchmarks/data/tpch_" + input_size_mb + "MB/" + filename + ".tbl";
    checkForErrors(evalStorage("Load"_(table, path)));
  }
}

#pragma endregion loading

auto& tpch_queries() {
  static map<string, ComplexExpression> queries;
  if(queries.empty()) {


    if (false) {
      queries.try_emplace(
        "test-arrow-storage",
          "foo"_("LINEITEM"_)
      );
      return queries;
    }

// ========================== Macro-benchmarks ==========================


// we skip order by
// 1998-12-01 - 90 days = 1998-09-01 -> 10470

// select
//   l_returnflag,
//   l_linestatus,
//   sum(l_quantity) as sum_qty,
//   sum(l_extendedprice) as sum_base_price,
//   sum(l_extendedprice*(1-l_discount)) as sum_disc_price,
//   sum(l_extendedprice*(1-l_discount)*(1+l_tax)) as sum_charge,
//   avg(l_quantity) as avg_qty,
//   avg(l_extendedprice) as avg_price,
//   avg(l_discount) as avg_disc,
//   count(*) as count_order
// from
//   lineitem
// where
//   l_shipdate <= '1998-09-01'
// group by
//   l_returnflag,
//   l_linestatus
 
    queries.try_emplace(
      "q1-tpch",
        "aggregate"_(
          "project"_(
            "project"_(
              "select"_(
                "LINEITEM"_,
                string_list("l_shipdate"),
                string_list("<="),
                int_list(10470)
              ),
              string_list(), string_list(), string_list(), 
              int_list(1, 1), string_list("-", "+"), string_list("l_discount", "l_tax"), string_list("x", "y"),
              string_list("l_returnflag", "l_linestatus", "l_quantity", "l_extendedprice", "l_discount", "x", "y"),
              string_list("l_returnflag", "l_linestatus", "l_quantity", "l_extendedprice", "l_discount", "x", "y")
            ),
            string_list(), string_list(), string_list(), 
            string_list("l_extendedprice", "z"), string_list("*", "*"), string_list("x", "y"), string_list("z", "w"),
            string_list("l_returnflag", "l_linestatus", "l_quantity", "l_extendedprice", "l_discount", "z", "w"),
            string_list("l_returnflag", "l_linestatus", "l_quantity", "l_extendedprice", "l_discount", "z", "w")
          ),
          string_list("l_returnflag", "l_linestatus"),
          string_list("sum", "sum", "sum", "sum", "avg", "avg", "avg", "count"),
          string_list("l_quantity", "l_extendedprice", "z", "w", "l_quantity", "l_extendedprice", "l_discount", "l_quantity"),
          string_list("sum_qty", "sum_base_price", "sum_disc_price", "sum_charge", "avg_qty", "avg_price", "avg_disc", "count_order")
        )
    );

    if (false) {
      return queries;
    }

// we skip order by
// we substitute:
// BUILDING -> 0
// 1995-03-15 -> 9204

// select
//   l_orderkey,
//   sum(l_extendedprice*(1-l_discount)) as revenue,
//   o_orderdate,
//   o_shippriority
// from
//   customer,
//   orders,
//   lineitem
// where
//   c_custkey = o_custkey
//   and l_orderkey = o_orderkey
//   and c_mktsegment = 'BUILDING' -- 0
//   and o_orderdate < '1995-03-15' -- 9204
//   and l_shipdate > '1995-03-15' -- 9204
// group by
//   l_orderkey,
//   o_orderdate,
//   o_shippriority

    queries.try_emplace(
      "q3-tpch",
      "aggregate"_(
        "project"_(
          "project"_(
            "select"_(
              "equi_join"_(
                "equi_join"_(
                  "CUSTOMER"_,
                  "ORDERS"_,
                  string_list("c_custkey"),
                  string_list("o_custkey")
                ),
                "LINEITEM"_,
                string_list("o_orderkey"),
                string_list("l_orderkey")
              ),
              string_list("o_orderdate", "l_shipdate", "c_mktsegment"),
              string_list("<", ">", "=="),
              int_list(9204, 9204, 0)
            ),
            string_list(), string_list(), string_list(), 
            int_list(1), string_list("-"), string_list("l_discount"), string_list("x"),
            string_list("l_orderkey", "l_extendedprice", "x", "o_orderdate", "o_shippriority", "o_orderkey"),
            string_list("l_orderkey", "l_extendedprice", "x", "o_orderdate", "o_shippriority", "o_orderkey")
          ),
          string_list(), string_list(), string_list(),
          string_list("l_extendedprice"), string_list("*"), string_list("x"), string_list("y"),
          string_list("l_orderkey", "y", "o_orderdate", "o_shippriority", "o_orderkey"),
          string_list("l_orderkey", "y", "o_orderdate", "o_shippriority", "o_orderkey")
        ),
        string_list("l_orderkey", "o_orderkey", "o_shippriority"),
        string_list("sum"),
        string_list("y"),
        string_list("revenue")
      )
    );

// we substitute:
// 1994-01-01 -> 8766
// 1995-01-01 -> 9131

// select
//   sum(l_extendedprice*l_discount) as revenue
// from
//   lineitem
// where
//   l_shipdate >= '1994-01-01' -- 8766
//   and l_shipdate < '1995-01-01' -- 9131
//   and l_discount > 0.05 
//   and l_discount < 0.07
//   and l_quantity < 24

    queries.try_emplace(
      "q6-tpch",
      "project"_(
        "project"_(
          "select"_(
            "LINEITEM"_,
            string_list("l_shipdate", "l_shipdate", "l_discount", "l_discount", "l_quantity"),
            string_list(">=", "<", ">", "<", "<"),
            double_list(8766, 9131, 0.05, 0.07, 24)
          ),
          string_list(), string_list(), string_list(),
          string_list("l_extendedprice"), string_list("*"), string_list("l_discount"), string_list("x"),
          string_list("x"), string_list("x")
        ),
        string_list("sum"), string_list("x"), string_list("revenue"),
        string_list(), string_list(), string_list(), string_list(), 
        string_list("revenue"), string_list("revenue") 
      )
    );

// we skip order by
// we skip string pattern matching
// we skip extracting year from date

// select
//   nation,
//   o_orderdate,
//   sum(amount) as sum_profit
// from (
//   select
//     n_name as nation,
//     o_orderdate,
//     l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
//   from
//     part,
//     supplier,
//     lineitem,
//     partsupp,
//     orders,
//     nation
//   where
//     s_suppkey = l_suppkey
//     and ps_suppkey = l_suppkey
//     and ps_partkey = l_partkey
//     and p_partkey = l_partkey
//     and o_orderkey = l_orderkey
//     and s_nationkey = n_nationkey
//   )
// group by
//   nation,
//   o_orderdate
  
    queries.try_emplace(
      "q9-tpch",
      "aggregate"_(
        "project"_(
          "project"_(
            "equi_join"_(
              "equi_join"_(
                "equi_join"_(
                  "equi_join"_(
                    "equi_join"_(
                      "SUPPLIER"_,
                      "LINEITEM"_,
                      string_list("s_suppkey"),
                      string_list("l_suppkey")
                    ),
                    "PARTSUPP"_,
                    string_list("l_suppkey", "l_partkey"),
                    string_list("ps_suppkey", "ps_partkey")
                  ),
                  "PART"_,
                  string_list("l_partkey"),
                  string_list("p_partkey")
                ),
                "ORDERS"_,
                string_list("l_orderkey"),
                string_list("o_orderkey")
              ),
              "NATION"_,
              string_list("s_nationkey"),
              string_list("n_nationkey")
            ),
            string_list(), string_list(), string_list(),
            int_list(1),
            string_list("-"),
            string_list("l_discount"),
            string_list("x"),
            string_list("l_extendedprice", "ps_supplycost", "x", "l_quantity", "n_name", "o_orderdate"),
            string_list("l_extendedprice", "ps_supplycost", "x", "l_quantity", "n_name", "o_orderdate")
          ),
          string_list(), string_list(), string_list(),
          string_list("l_extendedprice", "ps_supplycost", "y"),
          string_list("*", "*", "-"), 
          string_list("x", "l_quantity", "z"), 
          string_list("y", "z", "amount"),
          string_list("n_name", "o_orderdate", "amount"),
          string_list("nation", "o_orderdate", "amount")
        ),
        string_list("nation", "o_orderdate"),
        string_list("sum"),
        string_list("amount"),
        string_list("sum_profit")
      )
    );

// we substitute:
// a sub-query that evaluates to a scalar value -> 5.1
// Brand#23 -> 0
// MED BOX -> 0

// select
//   sum(l_extendedprice) / 7.0 as avg_yearly
// from
//   lineitem,
//   part
// where
//   p_partkey = l_partkey
//   and l_quantity < 5.1
//   and p_brand = 'Brand#23' -- 0
//   and p_container = 'MED BOX' -- 0
  
  queries.try_emplace(
    "q17-tpch",
    "project"_(
      "select"_(
        "equi_join"_(
          "LINEITEM"_,
          "PART"_,
          string_list("l_partkey"),
          string_list("p_partkey")
        ),
        string_list("l_quantity", "p_brand", "p_container"),
        string_list("<", "==", "=="),
        double_list(5.1, 0, 0)
      ),
      string_list("sum"), string_list("l_extendedprice"), string_list("x"),
      string_list("x"), string_list("/"), double_list(7.0), string_list("avg_yearly"),
      string_list("avg_yearly"), string_list("avg_yearly")
    )
  );

// ========================== Micro-benchmarks ==========================

// based on Q6

// select
//   sum(l_extendedprice*l_discount) as revenue
// from
//   lineitem

    queries.try_emplace(
      "project",
      "project"_(
        "project"_(
          "LINEITEM"_,
          string_list(), string_list(), string_list(),
          string_list("l_extendedprice"), string_list("*"), string_list("l_discount"), string_list("x"),
          string_list("x"), string_list("x")
        ),
        string_list("sum"), string_list("x"), string_list("revenue"),
        string_list(), string_list(), string_list(), string_list(), 
        string_list("revenue"), string_list("revenue") 
      )
    );

// based on Q6

// we substitute:
// 1994-01-01 -> 8766
// 1995-01-01 -> 9131

// select
//   *
// from
//   lineitem
// where
//   l_shipdate >= '1994-01-01' -- 8766
//   and l_shipdate < '1995-01-01' -- 9131
//   and l_discount > 0.05 
//   and l_discount < 0.07
//   and l_quantity < 24

    queries.try_emplace(
      "select",
      "select"_(
        "LINEITEM"_,
        string_list("l_shipdate", "l_shipdate", "l_discount", "l_discount", "l_quantity"),
        string_list(">=", "<", ">", "<", "<"),
        double_list(8766, 9131, 0.05, 0.07, 24)
      )
    );

// based on Q3

// select
//   *
// from
//   orders,
//   lineitem
// where
//   l_orderkey = o_orderkey

    queries.try_emplace(
      "equi_join",
      "equi_join"_(
        "ORDERS"_,
        "LINEITEM"_,
        string_list("o_orderkey"),
        string_list("l_orderkey")
      )
    );

// based on Q1

// select
//   l_returnflag,
//   l_linestatus,
//   sum(l_quantity) as sum_qty,
//   sum(l_extendedprice) as sum_base_price,
//   avg(l_quantity) as avg_qty,
//   avg(l_extendedprice) as avg_price,
//   avg(l_discount) as avg_disc,
//   count(*) as count_order
// from
//   lineitem
// group by
//   l_returnflag,
//   l_linestatus

    queries.try_emplace(
      "aggregate",
      "aggregate"_(
        "LINEITEM"_,
        string_list("l_returnflag", "l_linestatus"),
        string_list("sum", "sum", "avg", "avg", "avg", "count"),
        string_list("l_quantity", "l_extendedprice", "l_quantity", "l_extendedprice", "l_discount", "l_quantity"),
        string_list("sum_qty", "sum_base_price", "avg_qty", "avg_price", "avg_disc", "count_order")
      )
    );

  }
  return queries;
}

#pragma region queries

ComplexExpression python_import_numpy() {
  return "Python_globals"_(R"(
import numpy as np
import copy
)"_);
}

auto& rand_queries() {
  static map<string, ComplexExpression> queries;
  
    if(queries.empty()) {
    queries.try_emplace(
      "_1_data_in", 
      "Python"_(""_, "Where"_("rand_table_python"_, "rand_table_boss"_))
    );
    queries.try_emplace(
      "_2_round_trip", 
      "And"_(
        "Python"_(""_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
        "get_python_var"_("rand_table_python"_)
      )
    );
    // queries.try_emplace(
    //   "_2_5_round_trip_w_copy", 
    //   "And"_(
    //     "Python"_("foo = copy.deepcopy(rand_table_python)"_, "Where"_("rand_table_python"_, "rand_table_boss"_)),
    //     "get_python_var"_("foo"_)
    //   )
    // );
    queries.try_emplace(
      "_3_materialise_columns", 
        "And"_(
          "Python"_(R"(
table = rand_table_python['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = [np.concatenate(spans)]

# print('table_cpy', table_cpy, sep='\n')
# res_table_python = {'table': table_cpy, 'matrix': None}
          )"_, "Where"_("rand_table_python"_, "rand_table_boss"_))
          // "get_python_var"_("rand_table_python"_),
          // "get_python_var"_("res_table_python"_)
        )
    );
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

# m_wrapper = {'data': m, 'col_names': list(table.keys())}
# res_table_python = {'table': None, 'matrix': m_wrapper}
        )"_, "Where"_("rand_table_python"_, "rand_table_boss"_))
        // "get_python_var"_("rand_table_python"_),
        // "get_python_var"_("res_table_python"_)
      )
    );
    queries.try_emplace(
      "_5_matrix_vector_product", 
      "And"_(
        "Python"_(R"(
table = rand_table_python['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)

m = np.stack(list(table_cpy.values()), axis=0) # matrix row = table column
w = np.array(
  [8.41, 3.14, 5.29, -3.81, 0.03, -6.42, -8.37, 2.78], 
  dtype=np.float64).reshape((8, 1))
res = w.T @ m
#print('m.shape', m.shape)
#print('res.shape', res.shape)
#print('res', res, sep='\n')

# res_wrapper = {'data': res, 'col_names': ['aggregate_value']}
# res_table_python = {'table': None, 'matrix': res_wrapper}
        )"_, "Where"_("rand_table_python"_, "rand_table_boss"_))
        // "get_python_var"_("rand_table_python"_),
        // "get_python_var"_("res_table_python"_)
      )
    );
    queries.try_emplace(
      "_6_matrix_matrix_product", 
      "And"_(
        "Python"_(R"(
table = rand_table_python['table']
table_cpy = dict()
for k in table.keys():
  spans = table[k]
  table_cpy[k] = np.concatenate(spans)

m = np.stack(list(table_cpy.values()), axis=0) # matrix row = table column
res = m @ m.T
# print('res', res, sep='\n')

# res_wrapper = {'data': res, 'col_names': list(table.keys())}
# res_table_python = {'table': None, 'matrix': res_wrapper}
        )"_, "Where"_("rand_table_python"_, "rand_table_boss"_))
        // "get_python_var"_("rand_table_python"_),
        // "get_python_var"_("res_table_python"_)
      )
    );
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
  map<string, ComplexExpression> &query_names_exprs) {
  auto eval = getEvaluateLambda();
  auto eval_numpy = getEvaluateBaselineLambda();
  cout << endl;

  for (const auto& [table_name, table_path] : table_names_paths) {
    create_and_load_table(table_name, table_path);

    csv << table_names[table_name];

    for (const auto& [query_name, query_expr] : query_names_exprs) {
      for (int j = 0; j < 1; j++) {
        cout << "========== start " << table_name << " " << query_name << " ==========" << endl;

        eval_numpy("reset_python_dict"_);
  
        if (true) {
          // cout << shallowCopy(query_expr) << endl;
          // cout << endl;
          // cout << evalStorage(shallowCopy(query_expr)) << endl;
          // cout << endl;
          auto res = eval(shallowCopy(query_expr));
          // cout << "res" << endl;
          // cout << res << endl;
          // cout << endl;
        }

        if (false) {
          const chrono::seconds time_warmup = 3s;
          const ull warmup_iters = 1;
          chrono::high_resolution_clock::time_point warmup_start = chrono::high_resolution_clock::now();
          chrono::high_resolution_clock::time_point warmup_end_time = warmup_start + time_warmup;
          chrono::high_resolution_clock::time_point warmup_timestamp = warmup_start;
          for (ull i = 0; i < warmup_iters || warmup_timestamp < warmup_end_time; i++) {
            auto res = eval(shallowCopy(query_expr));
            benchmark::DoNotOptimize(res);
            warmup_timestamp = chrono::high_resolution_clock::now();
          }

          const chrono::seconds time_test = 10s;
          const ull test_iters = 1;
          chrono::high_resolution_clock::time_point test_start = chrono::high_resolution_clock::now();
          chrono::high_resolution_clock::time_point test_end_time = test_start + time_test;
          chrono::high_resolution_clock::time_point test_timestamp = test_start;
          ull completed_iters = 0;
          for (completed_iters = 0; completed_iters < test_iters || test_timestamp < test_end_time; completed_iters++) {
            auto res = eval(shallowCopy(query_expr));
            benchmark::DoNotOptimize(res);
            test_timestamp = chrono::high_resolution_clock::now();
          }

          chrono::high_resolution_clock::time_point test_end = chrono::high_resolution_clock::now();
          chrono::nanoseconds elapsed_time = chrono::duration_cast<chrono::nanoseconds>(test_end - test_start);
          chrono::nanoseconds avg_time = elapsed_time / completed_iters;

          print_elapsed_time(avg_time);
          cout << endl;
          cout << "end " << " " << query_name << endl;
          cout << endl;
          csv << "," << avg_time.count();
        }
      }

    }
    csv << endl;

    unload_all_tables();
  }

  string csv_str = csv.str();
  ofstream outfile(csv_path);
  outfile << csv_str;
  outfile.close();
}

void benchmark_loop_tpch(
  map<string, ComplexExpression> &query_names_exprs) {
  auto evalStorage = getEvaluateStorageLambda();
  auto eval = getEvaluateLambda();
  cout << endl;
  std::unordered_set<std::string> micro_queries = {"project", "select", "equi_join", "aggregate"};

    for (const auto& [query_name, query_expr] : query_names_exprs) {
      cout << "========== start " << query_name << " ==========" << endl;

      if (true && (query_name != query_to_run)) {
        continue;
      }

      if (false && (query_name == "q3-tpch" || query_name == "q9-tpch")) {
        continue;
      }

      if (false && (micro_queries.find(query_name) == micro_queries.end())) {
        continue;
      }
 
      if (false) {
        cout << shallowCopy(query_expr) << endl;
        cout << endl;
        // cout << evalStorage(shallowCopy(query_expr)) << endl;
        // cout << endl;
        auto res = eval(shallowCopy(query_expr));
        // cout << "res" << endl;
        // cout << res << endl;
        // cout << endl;
      }

      if (true) {
        const chrono::seconds time_warmup = 3s;
        const ull warmup_iters = 1;
        chrono::high_resolution_clock::time_point warmup_start = chrono::high_resolution_clock::now();
        chrono::high_resolution_clock::time_point warmup_end_time = warmup_start + time_warmup;
        chrono::high_resolution_clock::time_point warmup_timestamp = warmup_start;
        for (ull i = 0; i < warmup_iters || warmup_timestamp < warmup_end_time; i++) {
          auto res = eval(shallowCopy(query_expr));
          benchmark::DoNotOptimize(res);
          warmup_timestamp = chrono::high_resolution_clock::now();
        }

        const chrono::seconds time_test = 10s;
        const ull test_iters = 1;
        chrono::high_resolution_clock::time_point test_start = chrono::high_resolution_clock::now();
        chrono::high_resolution_clock::time_point test_end_time = test_start + time_test;
        chrono::high_resolution_clock::time_point test_timestamp = test_start;
        ull completed_iters = 0;
        for (completed_iters = 0; completed_iters < test_iters || test_timestamp < test_end_time; completed_iters++) {
          auto res = eval(shallowCopy(query_expr));
          benchmark::DoNotOptimize(res);
          test_timestamp = chrono::high_resolution_clock::now();
        }

        chrono::high_resolution_clock::time_point test_end = chrono::high_resolution_clock::now();
        chrono::nanoseconds elapsed_time = chrono::duration_cast<chrono::nanoseconds>(test_end - test_start);
        chrono::nanoseconds avg_time = elapsed_time / completed_iters;

        print_elapsed_time(avg_time);
        cout << endl;
        cout << "end " << " " << query_name << endl;
        cout << endl;
      }
    }
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

void tpch_bench() {
  init_libraries();
  storageLibrary = librariesToTest[0];
  initStorageEngine_TPCH();
  auto eval = getEvaluateLambda();
  // eval(python_import_numpy());

  benchmark_loop_tpch(tpch_queries());
}

int main(int argc, char** argv) {
  for(int i = 0; i < argc; ++i) {
    if(std::string("--size") == argv[i]) {
      if(++i < argc) {
        input_size_mb = argv[i];
      }
    } else if(std::string("--query") == argv[i]) {
      if(++i < argc) {
        query_to_run = argv[i];
      }
    }
  }
  if (input_size_mb == "" || query_to_run == "") {
    throw runtime_error("provide --size and --query");
  }
  try {
    // init_and_run_benchmarks();
    tpch_bench();
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
