#include "BOSSNumPyEngine.hpp"

using namespace std;

using string_literals::operator"" s;
using boss::utilities::operator""_;
using boss::ComplexExpression;
using boss::Span;
using boss::Symbol;
using boss::expressions::ExpressionSpanArguments;

using boss::Expression;

namespace boss::engines::numpy {

Expression Engine::evaluate(Expression &&e) {
  cout << "expression is " << e << endl;

  return std::visit(
      boss::utilities::overload(
          [this](ComplexExpression &&expression) -> boss::Expression {
            auto [head, statics, dynamics, spans] =
                std::move(expression).decompose();

            cout << "complex expression" << endl;
            cout << "head is " << head.getName() << endl;

            for (auto &&arg : dynamics) {
              cout << "dynamic is " << arg << endl;
            }

            std::for_each(
                std::make_move_iterator(spans.begin()),
                std::make_move_iterator(spans.end()), [&](auto &&span) {
                  std::visit(
                      []<typename T>(boss::Span<T> &&typedSpan) -> void {
                        if constexpr (std::is_same_v<T, int32_t> ||
                                      std::is_same_v<T, int64_t> ||
                                      std::is_same_v<T, float_t> ||
                                      std::is_same_v<T, double_t> ||
                                      std::is_same_v<T, int32_t const> ||
                                      std::is_same_v<T, int64_t const> ||
                                      std::is_same_v<T, float_t const> ||
                                      std::is_same_v<T, double_t const>) {
                          std::for_each(
                              std::make_move_iterator(typedSpan.begin()),
                              std::make_move_iterator(typedSpan.end()),
                              [&](auto &&spanElement) {
                                cout << "span element " << spanElement << endl;
                              });
                        } else {
                          throw std::runtime_error(
                              "unsupported span type: " +
                              std::string(typeid(decltype(typedSpan)).name()));
                        }
                      },
                      std::move(span));
                });

            cout << endl;

            // if (head == "Project"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Select"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Join"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Group"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "As"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Where"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "And"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Sum"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Times"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Greater"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "Table"_)
            // {
            //   return std::move(expression);
            // }
            // else if (head == "List"_)
            // {
            //   return std::move(expression);
            // }

            // if (head == "List"_) {
            //   forward<decltype(spans)>(move(spans));
            // }

            std::transform(std::make_move_iterator(dynamics.begin()),
                           std::make_move_iterator(dynamics.end()),
                           dynamics.begin(), [this](auto &&arg) {
                             return evaluate(std::forward<decltype(arg)>(arg));
                           });
            return boss::ComplexExpression(
                std::move(head), {}, std::move(dynamics), std::move(spans));
          },
          [this](Symbol &&symbol) -> boss::Expression {
            cout << "symbol" << endl;
            cout << endl;
            return std::move(symbol);
          },
          [](auto &&arg) -> boss::Expression {
            cout << typeid(arg).name() << endl;
            cout << endl;
            return std::forward<decltype(arg)>(arg);
          }),
      std::move(e));
};

// Py_ArrayObject Engine::convertSpansToNumPy(ExpressionSpanArguments &&spans) {
//   // PyArray_SimpleNewFromData(nd, dims, typenum, data)
// }


void init_numpy() {
  Py_Initialize();
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wreturn-type"
  import_array();
  #pragma clang diagnostic pop
  if (PyErr_Occurred()) {
    throw std::runtime_error("Failed to import numpy Python module(s).");
  }
  assert(PyArray_API);
}

Engine::Engine() {
  init_numpy();
}

} // namespace boss::engines::numpy


static auto &enginePtr(bool initialise = true) {
  static auto engine = std::unique_ptr<boss::engines::numpy::Engine>();
  if (!engine && initialise) {
    engine.reset(new boss::engines::numpy::Engine());
  }
  return engine;
}

extern "C" BOSSExpression *evaluate(BOSSExpression *e) {
  static std::mutex m;
  std::lock_guard lock(m);
  auto *r = new BOSSExpression{enginePtr()->evaluate(std::move(e->delegate))};
  return r;
};

extern "C" void reset() { enginePtr(false).reset(nullptr); }
