#pragma once

#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_2_0_API_VERSION
#define NPY_TARGET_VERSION NPY_2_0_API_VERSION

#include "numpy/arrayobject.h"
#include <Python.h>

#include <BOSS.hpp>
#include <Expression.hpp>
#include <ExpressionUtilities.hpp>
#include <Utilities.hpp>

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <vector>
#include <functional>
#include <memory>
#include <cmath>
#include <stdio.h>
#include <variant>

using namespace std;

using PythonExpressionSystem = boss::expressions::generic::ExtensibleExpressionSystem<PyObject *>;
using AtomicExpression = PythonExpressionSystem::AtomicExpression;
using ComplexExpression = PythonExpressionSystem::ComplexExpression;
template <typename... T>
using ComplexExpressionWithStaticArguments =
    PythonExpressionSystem::ComplexExpressionWithStaticArguments<T...>;
using Expression = PythonExpressionSystem::Expression;
using ExpressionArguments = PythonExpressionSystem::ExpressionArguments;
using ExpressionSpanArguments = PythonExpressionSystem::ExpressionSpanArguments;
using ExpressionSpanArgument = PythonExpressionSystem::ExpressionSpanArgument;
using ExpressionBuilder = boss::utilities::ExtensibleExpressionBuilder<PythonExpressionSystem>;
static ExpressionBuilder operator""_(const char* name, size_t /*unused*/) {
  return ExpressionBuilder(name);
};
using boss::Span;
using boss::Symbol;

typedef unsigned long long ull;
const int ENGINE_SPAN_SIZE_BYTES = 1000000; // 1 million = 1 mb
// using MyExpression = std::variant<PythonExpressionSystem::Expression, PyObject *>;

namespace boss::engines::numpy {

class Engine {

public:
  Engine(Engine &) = delete;

  Engine &operator=(Engine &) = delete;

  Engine(Engine &&) = default;

  Engine &operator=(Engine &&) = delete;

  Engine(ull span_size_bytes);

  ~Engine();

  PythonExpressionSystem::Expression evaluate(PythonExpressionSystem::Expression &&e);
  boss::expressions::Expression evaluate_c(boss::expressions::Expression &&e);

private:
  PyObject *global_dict;
  PyObject *local_dict;
  PyObject *rel_alg;
  PyObject *main_module;

  ull span_size_bytes;

  void init_python_and_numpy();

  template <typename T>
  PythonExpressionSystem::ComplexExpression
  npy_matrix_to_table_helper(PyArrayObject *npy_matrix,
                                     PyObject *col_names);

  PythonExpressionSystem::ComplexExpression npy_matrix_to_table(PyArrayObject *npy_matrix,
                                                PyObject *col_names);
                                            
  PythonExpressionSystem::ExpressionSpanArgument numpy_arr_to_span(PyObject *npy_arr);
  PyObject *span_to_numpy_arr(PythonExpressionSystem::ExpressionSpanArgument &&arg);
  PythonExpressionSystem::ExpressionSpanArguments py_list_to_spans(PyObject *list);
  PyObject *spans_to_py_list(PythonExpressionSystem::ExpressionSpanArguments &&args);
  void reset_python_dict();

  PyObject *table_to_pywrapper(PythonExpressionSystem::ComplexExpression &&table_expr);
  PythonExpressionSystem::Expression pytable_to_table(PyObject *table_dict);
  PythonExpressionSystem::ComplexExpression pymatrix_to_table(PyObject *matrix_dict);
  PythonExpressionSystem::Expression pywrapper_to_table(PyObject *wrapper_dict);
  PyObject *table_to_pydict(PythonExpressionSystem::ComplexExpression &&table_expr);
  PythonExpressionSystem::Expression pydict_to_table(PyObject *table_dict);
};

PythonExpressionSystem::ComplexExpression create_random_table(int num_cols, ull table_size, ull span_size_bytes, vector<unique_ptr<vector<int>>> &span_ptrs);

} // namespace boss::engines::numpy
