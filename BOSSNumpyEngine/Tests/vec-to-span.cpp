#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_2_0_API_VERSION
#define NPY_TARGET_VERSION NPY_2_0_API_VERSION

#include "numpy/arrayobject.h"
#include <Python.h>

#include "../Source/BOSSNumpyEngine.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

using namespace std;
using intType = int32_t;
using boss::engines::numpy::Engine;
using boss::engines::numpy::create_random_table;
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
using boss::expressions::CloneReason;

using namespace std;

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
  cout << "end of numpy array" << endl;
}

ExpressionSpanArgument print_span_arg(ExpressionSpanArgument &&arg) {
  return visit(
      []<typename T>(Span<T> &&typed_span) -> ExpressionSpanArgument {
        if constexpr (is_same_v<T, int32_t> || is_same_v<T, int64_t> ||
                      is_same_v<T, float_t> || is_same_v<T, double_t>) {

          cout << "[";

          transform(make_move_iterator(typed_span.begin()),
                    make_move_iterator(typed_span.end()), typed_span.begin(),
                    [&](auto &&elem) {
                      cout << elem << ", ";
                      return forward<decltype(elem)>(elem);
                    });

          cout << "]" << endl;

          return forward<decltype(typed_span)>(typed_span);

        } else {
          throw runtime_error("unsupported span type: " +
                              string(typeid(decltype(typed_span)).name()));
        }
      },
      forward<decltype(arg)>(arg));
}

void print_vec(vector<int32_t> &vec) {
  for (int32_t i = 0; i < vec.size(); i++) {
    cout << vec[i] << ", ";
  }
  cout << endl;
}

void init_python_and_numpy() {
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

int main() {
    init_python_and_numpy();

    vector<int32_t> vec{1, 2, 3};

    auto span = boss::Span<int32_t>(vec.data(), vec.size(), [
      // to_destroy = std::move(vec)
      ]() {
      cout << "deleting span" << endl;
    });

    auto typenum = NPY_INT32;
    auto begin = reinterpret_cast<int32_t*>(span.begin());
    auto end = span.end();
    auto size = span.size();
    npy_intp dims[] = {static_cast<npy_intp>(size)};
    auto npy_arr = reinterpret_cast<PyArrayObject *>(PyArray_SimpleNewFromData(1, dims, typenum, begin));

    auto span_arg = move(print_span_arg(move(span)));
    print_vec(vec);
    print_1d_numpy_array(npy_arr);
    cout << endl;

    vec[0] = 42;

    span_arg = move(print_span_arg(move(span_arg)));
    print_vec(vec);
    print_1d_numpy_array(npy_arr);
    cout << endl;

    *reinterpret_cast<int32_t*>(PyArray_GETPTR1(npy_arr, 1)) = 13;

    span_arg = move(print_span_arg(move(span_arg)));
    print_vec(vec);
    print_1d_numpy_array(npy_arr);
    cout << endl;

    span = get<Span<int32_t>>(move(span_arg));
    begin = reinterpret_cast<int32_t *>(span.begin());
    *(begin + 2) = 420;

    span_arg = move(print_span_arg(move(span)));
    print_vec(vec);
    print_1d_numpy_array(npy_arr);
    cout << endl;

    Py_Finalize();

    return 0;
}
