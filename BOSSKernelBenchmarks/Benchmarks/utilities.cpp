#ifndef UTILITIES_CPP
#define UTILITIES_CPP

#include <BOSS.hpp>
#include <ExpressionUtilities.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstdlib>

#include "config.hpp"

using namespace boss::utilities;
using SpanArguments = boss::DefaultExpressionSystem::ExpressionSpanArguments;

namespace utilities {
static boss::ComplexExpression shallowCopy(boss::ComplexExpression const& e) {
  auto const& head = e.getHead();
  auto const& dynamics = e.getDynamicArguments();
  auto const& spans = e.getSpanArguments();
  boss::ExpressionArguments dynamicsCopy;
  std::transform(dynamics.begin(), dynamics.end(), std::back_inserter(dynamicsCopy),
                 [](auto const& arg) {
                   return std::visit(
                       boss::utilities::overload(
                           [&](boss::ComplexExpression const& expr) -> boss::Expression {
                             return shallowCopy(expr);
                           },
                           [](auto const& otherTypes) -> boss::Expression { return otherTypes; }),
                       arg);
                 });
  boss::expressions::ExpressionSpanArguments spansCopy;
  std::transform(spans.begin(), spans.end(), std::back_inserter(spansCopy), [](auto const& span) {
    return std::visit(
        [](auto const& typedSpan) -> boss::expressions::ExpressionSpanArgument {
          // just do a shallow copy of the span
          // the storage's span keeps the ownership
          // (since the storage will be alive until the query finishes)
          using SpanType = std::decay_t<decltype(typedSpan)>;
          using T = std::remove_const_t<typename SpanType::element_type>;
          if constexpr(std::is_same_v<T, bool>) {
            // this would still keep const spans for bools, need to fix later
            return SpanType(typedSpan.begin(), typedSpan.size(), []() {});
          } else {
            // force non-const value for now (otherwise expressions cannot be moved)
            auto* ptr = const_cast<T*>(typedSpan.begin()); // NOLINT
            return boss::Span<T>(ptr, typedSpan.size(), []() {});
          }
        },
        span);
  });
  return {head, {}, std::move(dynamicsCopy), std::move(spansCopy)};
}

static boss::Expression injectDebugInfoToSpans(boss::Expression&& expr) {
  return std::visit(
      boss::utilities::overload(
          [&](boss::ComplexExpression&& e) -> boss::Expression {
            auto [head, unused_, dynamics, spans] = std::move(e).decompose();
            boss::ExpressionArguments debugDynamics;
            debugDynamics.reserve(dynamics.size() + spans.size());
            std::transform(std::make_move_iterator(dynamics.begin()),
                           std::make_move_iterator(dynamics.end()),
                           std::back_inserter(debugDynamics), [](auto&& arg) {
                             return injectDebugInfoToSpans(std::forward<decltype(arg)>(arg));
                           });
            std::transform(
                std::make_move_iterator(spans.begin()), std::make_move_iterator(spans.end()),
                std::back_inserter(debugDynamics), [](auto&& span) {
                  return std::visit(
                      [](auto&& typedSpan) -> boss::Expression {
                        using Element = typename std::decay_t<decltype(typedSpan)>::element_type;
                        return boss::ComplexExpressionWithStaticArguments<std::string, int64_t>(
                            "Span"_, {typeid(Element).name(), typedSpan.size()}, {}, {});
                      },
                      std::forward<decltype(span)>(span));
                });
            return boss::ComplexExpression(std::move(head), {}, std::move(debugDynamics), {});
          },
          [](auto&& otherTypes) -> boss::Expression { return otherTypes; }),
      std::move(expr));
}
} // namespace utilities

auto getEvaluateLambda() {
  static auto lambda =
      []() -> std::function<boss::expressions::Expression(boss::expressions::Expression&&)> {
    if(USING_COORDINATOR_ENGINE) {
      return [](auto&& expression) {
        boss::ExpressionArguments libsArg;
        for(auto it = librariesToTest.begin() + 1; it != librariesToTest.end(); ++it)
          libsArg.emplace_back(*it);
        auto libsExpr = boss::ComplexExpression("List"_, std::move(libsArg));
        return boss::evaluate("DelegateBootstrapping"_(librariesToTest[0], std::move(libsExpr),
                                                       std::move(expression)));
      };
    } else {
      return [](auto&& expression) {
        boss::expressions::ExpressionSpanArguments spans;
        spans.emplace_back(boss::expressions::Span<std::string>(librariesToTest));
        return boss::evaluate("EvaluateInEngines"_(
            boss::ComplexExpression("List"_, {}, {}, std::move(spans)), std::move(expression)));
      };
    }
  }();
  return lambda;
}

auto getEvaluateStorageLambda() {
  static auto lambda = [](boss::Expression&& expression) mutable {
    return boss::evaluate("EvaluateInEngines"_("List"_(storageLibrary), std::move(expression)));
  };
  return lambda;
}

auto getEvaluateBaselineLambda() {
  static auto lambda = [](boss::Expression&& expression) mutable {
    return boss::evaluate("EvaluateInEngines"_("List"_(storageLibrary, librariesToTest.back()),
                                               std::move(expression)));
  };
  return lambda;
}

auto getErrorFoundLambda() {
  static auto lambda = [](auto&& result, auto const& queryName) {
    return false;
    if(!std::holds_alternative<boss::ComplexExpression>(result)) {
      return false;
    }
    if(std::get<boss::ComplexExpression>(result).getHead() == "Table"_) {
      return false;
    }
    if(std::get<boss::ComplexExpression>(result).getHead() == "Gather"_) {
      return false;
    }
    if(std::get<boss::ComplexExpression>(result).getHead() == "List"_) {
      return false;
    }
    if(std::get<boss::ComplexExpression>(result).getHead() == "Join"_) {
      return false;
    }
    std::cout << queryName << " Error: "
              << (VERY_VERBOSE_QUERY_OUTPUT ? std::move(result)
                                            : utilities::injectDebugInfoToSpans(std::move(result)))
              << std::endl;
    return true;
  };
  return lambda;
}

auto getCheckForErrorsLambda() {
  static auto lambda = [](auto&& output) {
    auto* maybeComplexExpr = std::get_if<boss::ComplexExpression>(&output);
    if(maybeComplexExpr == nullptr) {
      return;
    }
    if(maybeComplexExpr->getHead() == "ErrorWhenEvaluatingExpression"_) {
      std::cout << "Error: " << output << std::endl;
    }
  };
  return lambda;
}

void resetStorageEngine() {
  auto evalStorage = getEvaluateStorageLambda();
  evalStorage("DropTable"_("BIXI"_));
}

size_t getNumberOfRowsInTable(std::string& filepath) {
  std::ifstream file(filepath);
  if(!file.is_open()) {
    std::cerr << "Error: Unable to open file " << filepath << std::endl;
    return 0;
  }

  size_t rowCount = 0;
  std::string line;
  while(std::getline(file, line)) {
    ++rowCount;
  }

  file.close();
  return rowCount;
}

template<typename T>
SpanArguments loadVectorIntoSpans(std::vector<T> vector) {
  SpanArguments spans;
  auto remainingTuples = vector.size();
  auto start = 0;
  while(remainingTuples > 0) {
    auto spanLength = std::min(SPAN_SIZE_FOR_NON_ARROW, remainingTuples);
    spans.emplace_back(boss::Span<T>(std::vector(vector.begin() + start, vector.begin() + start + spanLength)));
    start += spanLength;
    remainingTuples -= spanLength;
  }
  return spans;
}

#endif // UTILITIES_CPP