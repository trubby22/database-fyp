#pragma once

// #define NPY_NO_DEPRECATED_API NPY_2_0_API_VERSION

// #include <Python.h>
// #include "numpy/arrayobject.h"

#include <BOSS.hpp>
#include <Expression.hpp>
#include <ExpressionUtilities.hpp>
#include <Utilities.hpp>

#include <iostream>
#include <mutex>

namespace boss::engines::numpy {

class Engine {

public:
  Engine(Engine&) = delete;

  Engine& operator=(Engine&) = delete;

  Engine(Engine&&) = default;

  Engine& operator=(Engine&&) = delete;

  Engine() = default;

  ~Engine() = default;

  boss::Expression evaluate(boss::Expression&& e);

private:
};

} // namespace boss::engines::numpy
