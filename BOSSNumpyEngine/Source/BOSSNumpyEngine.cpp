#include "BOSSNumpyEngine.hpp"

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
  int j = 0;
  for (npy_intp i = 0; i < size; ++i) {
    auto val = *static_cast<T *>(PyArray_GETPTR1(array, i));
    cout << val;
    if (i < size - 1) {
      cout << ", ";
    }
    j += 1;
    if (j >= 10) {
      break;
    }
  }
  cout << "]" << endl;
}

void print_1d_numpy_array(PyArrayObject *array) {
  cout << "printing numpy array" << endl;
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

template <typename T> NPY_TYPES boss_type_to_numpy() {
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

template <typename T> boss::Span<T> create_boss_span(PyArrayObject *npy_arr) {
  auto *data = static_cast<T *>(PyArray_DATA(npy_arr));
  auto length = PyArray_SIZE(npy_arr);
  return boss::Span<T>(data, length, [foo = std::move(*npy_arr)]() {});
}

ExpressionSpanArgument convert_numpy_to_span_arg(PyArrayObject *npy_arr) {
  ExpressionSpanArgument *result;

  int typenum = PyArray_TYPE(npy_arr);
  void *data = PyArray_DATA(npy_arr);

  switch (typenum) {
  case NPY_INT32:
    return create_boss_span<int32_t>(npy_arr);
    break;
  case NPY_INT64:
    return create_boss_span<int64_t>(npy_arr);
    break;
  case NPY_FLOAT:
    return create_boss_span<float_t>(npy_arr);
    break;
  case NPY_DOUBLE:
    return create_boss_span<double_t>(npy_arr);
    break;
  default:
    throw runtime_error("shouldn't happen");
    break;
  }
}

ExpressionSpanArguments
convert_vector_of_numpy_to_span_args(vector<PyArrayObject *> &&vec) {
  ExpressionSpanArguments result;
  result.reserve(vec.size());
  std::transform(
      std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()),
      std::back_inserter(result), [](auto &&elem) {
        auto span_arg =
            convert_numpy_to_span_arg(forward<decltype(elem)>(move(elem)));
        return span_arg;
      });
  return result;
}

PyArrayObject *convert_span_arg_to_numpy(ExpressionSpanArgument &&arg) {
  PyArrayObject *result;
  visit(
      [&result]<typename T>(boss::Span<T> &&typedSpan) {
        if constexpr (is_same_v<T, int32_t> || is_same_v<T, int64_t> ||
                      is_same_v<T, float_t> || is_same_v<T, double_t>) {

          auto typenum = boss_type_to_numpy<T>();
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

vector<PyArrayObject *>
convert_span_args_to_numpy(ExpressionSpanArguments &&args) {
  vector<PyArrayObject *> numpy_arrs;
  for_each(make_move_iterator(args.begin()), make_move_iterator(args.end()),
           [&](auto &&arg) {
             auto numpyArr =
                 convert_span_arg_to_numpy(forward<decltype(arg)>(move(arg)));
             numpy_arrs.push_back(numpyArr);
           });
  return numpy_arrs;
}

Expression Engine::evaluate(Expression &&e) {
  return visit(
      boss::utilities::overload(
          [this](ComplexExpression &&expression) -> boss::Expression {
            auto [head, statics, dynamics, spans] =
                move(expression).decompose();

            cout << head.getName() << endl;

            if (head == "List"_) {
              cout << "we have a list!" << endl;
              auto numpy_arrs = convert_span_args_to_numpy(
                  forward<decltype(spans)>(move(spans)));
              spans = convert_vector_of_numpy_to_span_args(
                  forward<decltype(numpy_arrs)>(move(numpy_arrs)));
            }

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
            cout << "symbol " << name << endl;

            return move(symbol);
          },
          [](auto &&arg) -> boss::Expression {
            cout << "other type " << typeid(arg).name() << endl;

            return forward<decltype(arg)>(move(arg));
          }),
      move(e));
};

void init_numpy() {
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
