#pragma once

#include <BOSS.hpp>
#include <Expression.hpp>
#include <ExpressionUtilities.hpp>
#include <Utilities.hpp>

#include <iostream>

namespace boss::engines::numpy {

class Engine {

public:
  Engine(Engine&) = delete;

  Engine& operator=(Engine&) = delete;

  Engine(Engine&&) = default;

  Engine& operator=(Engine&&) = delete;

  Engine();

  ~Engine();

  boss::Expression evaluate(boss::Expression&& e);

private:
};

} // namespace boss::engines::numpy
