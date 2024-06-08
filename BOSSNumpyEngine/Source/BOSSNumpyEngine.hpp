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

using namespace std;
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

typedef unsigned long long ull;

namespace boss::engines::numpy {

class Engine {

public:
  Engine(Engine &) = delete;

  Engine &operator=(Engine &) = delete;

  Engine(Engine &&) = default;

  Engine &operator=(Engine &&) = delete;

  Engine(ull span_size_bytes);

  ~Engine();

  boss::Expression evaluate(boss::Expression &&e);

private:
  PyObject *global_dict;
  PyObject *local_dict;
  PyObject *rel_alg;
  PyObject *main_module;

  ull span_size_bytes;

  void init_python_and_numpy();

  template <typename T>
  ComplexExpression
  npy_matrix_to_table_helper(PyArrayObject *npy_matrix,
                                     PyObject *col_names);

  ComplexExpression npy_matrix_to_table(PyArrayObject *npy_matrix,
                                                PyObject *col_names);
                                            
  ExpressionSpanArgument numpy_arr_to_span(PyObject *npy_arr);
  PyObject *span_to_numpy_arr(ExpressionSpanArgument &&arg);
  ExpressionSpanArguments py_list_to_spans(PyObject *list);
  PyObject *spans_to_py_list(ExpressionSpanArguments &&args);
  void reset_python_dict();
};

ComplexExpression create_random_table(int num_cols, ull table_size, ull span_size_bytes, vector<unique_ptr<vector<int>>> &span_ptrs);

} // namespace boss::engines::numpy
