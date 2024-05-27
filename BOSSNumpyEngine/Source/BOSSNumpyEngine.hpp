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

namespace boss::engines::numpy {

class Engine {

public:
  Engine(Engine &) = delete;

  Engine &operator=(Engine &) = delete;

  Engine(Engine &&) = default;

  Engine &operator=(Engine &&) = delete;

  Engine(size_t span_size);

  ~Engine() = default;

  boss::Expression evaluate(boss::Expression &&e);

private:
  PyObject *global_dict;
  size_t span_size;

  void init_python_and_numpy();

  template <typename T>
  ComplexExpression
  npy_matrix_to_table_helper(PyArrayObject *npy_matrix,
                                     PyObject *col_names);

  ComplexExpression npy_matrix_to_table(PyArrayObject *npy_matrix,
                                                PyObject *col_names);
};

} // namespace boss::engines::numpy
