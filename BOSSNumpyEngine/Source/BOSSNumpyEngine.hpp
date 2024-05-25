#pragma once

#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_2_0_API_VERSION
#define NPY_TARGET_VERSION NPY_2_0_API_VERSION

#include <Python.h>
#include "numpy/arrayobject.h"

#include <BOSS.hpp>
#include <Expression.hpp>
#include <ExpressionUtilities.hpp>
#include <Utilities.hpp>

#include <stdexcept>
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <unordered_map>

namespace boss::engines::numpy {

class Engine {

public:
  Engine(Engine&) = delete;

  Engine& operator=(Engine&) = delete;

  Engine(Engine&&) = default;

  Engine& operator=(Engine&&) = delete;

  Engine();

  ~Engine() = default;

  boss::Expression evaluate(boss::Expression&& e);

private:
  PyObject *global_dict = PyDict_New();
};

} // namespace boss::engines::numpy
