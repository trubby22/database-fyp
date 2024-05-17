#include "BOSSNumPyEngine.hpp"

using namespace std;

using string_literals::operator"" s;
using boss::utilities::operator""_;
using boss::ComplexExpression;
using boss::Span;
using boss::Symbol;
using boss::expressions::ExpressionSpanArgument;
using boss::expressions::ExpressionSpanArguments;

using boss::Expression;

namespace boss::engines::numpy {

template <typename T> void print_1d_numpy_array_helper(PyArrayObject *array) {
  auto size = PyArray_DIM(array, 0);
  cout << "[";
  for (npy_intp i = 0; i < size; ++i) {
    auto val = *static_cast<T *>(PyArray_GETPTR1(array, i));
    cout << val;
    if (i < size - 1) {
      cout << ", ";
    }
  }
  cout << "]" << endl;
}

void print_1d_numpy_array(PyArrayObject *array) {
  int dtype = PyArray_TYPE(array);
  switch (dtype) {
  case NPY_INT32: {
    print_1d_numpy_array_helper<int32_t>(array);
    break;
  }
  case NPY_INT64: {
    print_1d_numpy_array_helper<int64_t>(array);
    break;
  }
  case NPY_FLOAT: {
    print_1d_numpy_array_helper<float_t>(array);
    break;
  }
  case NPY_DOUBLE: {
    print_1d_numpy_array_helper<double_t>(array);
    break;
  }
  default: {
    throw logic_error("dtype is not as expected");
    break;
  }
  }
}

template <typename T> NPY_TYPES bossTypeToNumPy() {
  if constexpr (is_same_v<T, int32_t>) {
    return NPY_INT32;
  } else if constexpr (is_same_v<T, int64_t>) {
    return NPY_INT64;
  } else if constexpr (is_same_v<T, float_t>) {
    return NPY_FLOAT;
  } else if constexpr (is_same_v<T, double_t>) {
    return NPY_DOUBLE;
  } else {
    throw runtime_error("unsupported type: " + string(typeid(T).name()));
  }
}

PyArrayObject *convertSpanArgToNumPy(ExpressionSpanArgument &&arg) {
  PyArrayObject *result;
  visit(
      [&result]<typename T>(boss::Span<T> &&typedSpan) {
        if constexpr (is_same_v<T, int32_t> || is_same_v<T, int64_t> ||
                      is_same_v<T, float_t> || is_same_v<T, double_t>) {

          auto typenum = bossTypeToNumPy<T>();
          auto begin = typedSpan.begin();
          auto end = typedSpan.end();
          auto size = typedSpan.size();
          npy_intp dims[] = {static_cast<npy_intp>(size)};

          auto foo = PyArray_SimpleNewFromData(1, dims, typenum, begin);
          result = reinterpret_cast<PyArrayObject *>(foo);
        } else {
          throw runtime_error("unsupported span type: " +
                              string(typeid(decltype(typedSpan)).name()));
        }
      },
      move(arg));
  return result;
}

vector<PyArrayObject *> convertSpanArgsToNumPy(ExpressionSpanArguments &&args) {
  vector<PyArrayObject *> numpyArrs;
  for_each(make_move_iterator(args.begin()), make_move_iterator(args.end()),
           [&](auto &&arg) {
             auto numpyArr =
                 convertSpanArgToNumPy(forward<decltype(arg)>(move(arg)));
             numpyArrs.push_back(numpyArr);
           });
  return numpyArrs;
}

Expression Engine::evaluate(Expression &&e) {
  // cout << "expression is " << e << endl;

  return visit(
      boss::utilities::overload(
          [this](ComplexExpression &&expression) -> boss::Expression {
            auto [head, statics, dynamics, spans] =
                move(expression).decompose();

            // cout << "complex expression" << endl;
            // cout << "head is " << head.getName() << endl;

            // for (auto &&arg : dynamics) {
            //   cout << "dynamic is " << arg << endl;
            // }

            // for_each(make_move_iterator(spans.begin()),
            //          make_move_iterator(spans.end()), [&](auto &&span) {
            //            visit(
            //                []<typename T>(boss::Span<T> &&typedSpan) -> void {
            //                  if constexpr (is_same_v<T, int32_t> ||
            //                                is_same_v<T, int64_t> ||
            //                                is_same_v<T, float_t> ||
            //                                is_same_v<T, double_t> ||
            //                                is_same_v<T, int32_t const> ||
            //                                is_same_v<T, int64_t const> ||
            //                                is_same_v<T, float_t const> ||
            //                                is_same_v<T, double_t const>) {
            //                    for_each(make_move_iterator(typedSpan.begin()),
            //                             make_move_iterator(typedSpan.end()),
            //                             [&](auto &&spanElement) {
            //                               cout << "span element " << spanElement
            //                                    << endl;
            //                             });
            //                  } else {
            //                    throw runtime_error(
            //                        "unsupported span type: " +
            //                        string(typeid(decltype(typedSpan)).name()));
            //                  }
            //                },
            //                move(span));
            //          });

            // if (head == "Project"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Select"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Join"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Group"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "As"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Where"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "And"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Sum"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Times"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Greater"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "Table"_)
            // {
            //   return move(expression);
            // }
            // else if (head == "List"_)
            // {
            //   return move(expression);
            // }

            if (head == "List"_) {
              cout << "we have a list!" << endl;
              auto numpyArrs =
                  convertSpanArgsToNumPy(forward<decltype(spans)>(move(spans)));
              for_each(
                  make_move_iterator(numpyArrs.begin()),
                  make_move_iterator(numpyArrs.end()),
                  [&](auto &&numpyArr) { print_1d_numpy_array(numpyArr); });
            }
            
            cout << endl;

            transform(make_move_iterator(dynamics.begin()),
                      make_move_iterator(dynamics.end()), dynamics.begin(),
                      [this](auto &&arg) {
                        return evaluate(forward<decltype(arg)>(move(arg)));
                      });
            return boss::ComplexExpression(move(head), {}, move(dynamics),
                                           move(spans));
          },
          [this](Symbol &&symbol) -> boss::Expression {
            auto name = symbol.getName();
            // cout << "symbol " << name << endl;
            // cout << endl;
            return move(symbol);
          },
          [](auto &&arg) -> boss::Expression {
            // cout << "other type " << typeid(arg).name() << endl;
            // cout << endl;
            return forward<decltype(arg)>(move(arg));
          }),
      move(e));
};

void init_numpy() {
  cout << "init numpy" << endl;
  Py_Initialize();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
  import_array();
#pragma clang diagnostic pop
  if (PyErr_Occurred()) {
    throw runtime_error("Failed to import numpy Python module(s).");
  }
  assert(PyArray_API);
}

Engine::Engine() { init_numpy(); }

} // namespace boss::engines::numpy

static auto &enginePtr(bool initialise = true) {
  static auto engine = unique_ptr<boss::engines::numpy::Engine>();
  if (!engine && initialise) {
    engine.reset(new boss::engines::numpy::Engine());
  }
  return engine;
}

extern "C" BOSSExpression *evaluate(BOSSExpression *e) {
  static mutex m;
  lock_guard lock(m);
  auto *r = new BOSSExpression{enginePtr()->evaluate(move(e->delegate))};
  return r;
};

extern "C" void reset() { enginePtr(false).reset(nullptr); }
