#include "BOSSNumpyEngine.hpp"

using namespace std;

using string_literals::operator"" s;
using boss::utilities::operator""_;
using boss::ComplexExpression;
using boss::Span;
using boss::Symbol;
using boss::expressions::ExpressionSpanArgument;
using boss::expressions::ExpressionSpanArguments;
using boss::expressions::ExpressionArguments;

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

void print_py_list(PyObject *list) {
  cout << "py list" << endl;
  auto size = PyList_Size(list);
  for (int i = 0; i < size; i++) {
    auto npy_arr = PyList_GetItem(list, i);
    print_1d_numpy_array(npy_arr);
  }
  cout << "end of py list" << endl;
}

ExpressionSpanArgument print_span_arg(ExpressionSpanArgument &&arg) {
  return visit(
      []<typename T>(boss::Span<T> &&typed_span) -> ExpressionSpanArgument {
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

ExpressionSpanArguments print_span_args(ExpressionSpanArguments &&args) {
  cout << "span args" << endl;
  cout << "[";

  transform(make_move_iterator(args.begin()), make_move_iterator(args.end()),
            args.begin(), [](auto &&arg) {
              arg = print_span_arg(forward<decltype(arg)>(arg));
              cout << ", ";
              return move(arg);
            });

  cout << "]" << endl;
  cout << "end of span args" << endl;

  return forward<decltype(args)>(args);
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
  T *data = static_cast<T *>(PyArray_DATA(npy_arr));
  auto length = PyArray_SIZE(npy_arr);
  std::vector<T> v;
  v.assign(data, data + length);
  auto result = boss::Span<T>(std::move(v));
  return result;
}

ExpressionSpanArgument numpy_arr_to_span(PyArrayObject *npy_arr) {
  int typenum = PyArray_TYPE(npy_arr);

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

ExpressionSpanArguments py_list_to_spans(PyObject *list) {
  ExpressionSpanArguments result;
  auto size = PyList_Size(list);
  result.reserve(size);
  for (int i = 0; i < size; i++) {
    auto numpy_arr = PyList_GetItem(list, i);
    auto span_arg = numpy_arr_to_span(numpy_arr);
    result.emplace_back(move(span_arg));
  }
  return result;
}

PyArrayObject *span_to_numpy_arr(ExpressionSpanArgument &&arg) {
  PyArrayObject *result;
  visit(
      [&result]<typename T>(boss::Span<T> &&typed_span) {
        if constexpr (is_same_v<T, int32_t> || is_same_v<T, int64_t> ||
                      is_same_v<T, float_t> || is_same_v<T, double_t>) {

          auto typenum = boss_type_to_numpy<T>();
          auto begin = typed_span.begin();
          auto end = typed_span.end();
          auto size = typed_span.size();
          npy_intp dims[] = {static_cast<npy_intp>(size)};

          auto foo = PyArray_SimpleNewFromData(1, dims, typenum, begin);
          result = reinterpret_cast<PyArrayObject *>(foo);
        } else {
          throw runtime_error("unsupported span type: " +
                              string(typeid(decltype(typed_span)).name()));
        }
      },
      forward<decltype(arg)>(arg));
  return result;
}

PyObject *spans_to_py_list(ExpressionSpanArguments &&args) {
  PyObject *result = PyList_New(args.size()) auto it =
      make_move_iterator(args.begin());
  auto it_end = make_move_iterator(args.end());
  for (; it < it_end; it += 1) {
    auto numpy_arr = span_to_numpy_arr(*it);
    PyList_Append(result, numpy_arr);
  }
  return result;
}

Expression Engine::evaluate(Expression &&e) {
  return visit(
      boss::utilities::overload(
          [this](ComplexExpression &&expression) -> boss::Expression {
            auto [head, statics, dynamics, spans] =
                forward<decltype(expression)>(expression).decompose();

            cout << head.getName() << endl;

            if (head == "Python"_) {
              // head = Python
              auto it = std::make_move_iterator(dynamics.begin());
              auto script = static_cast<Symbol>(*it).getName().c_str();
              auto where = static_cast<ComplexExpression>(*++it);

              {
                auto [head, statics, dynamics, spans] = move(where).decompose();
                // head = Where
                auto it = make_move_iterator(dynamics.begin());
                auto it_end = make_move_iterator(dynamics.end());
                for (; it < it_end; it += 2) {
                  auto table_name = static_cast<Symbol>(*it).getName().c_str();
                  auto table_expr = static_cast<ComplexExpression>(*(it + 1));

                  {
                    // head = Table
                    auto [head, statics, dynamics, spans] =
                        move(table_expr).decompose();

                    PyObject *wrapper_dict = PyDict_New();
                    PyObject *table_dict = PyDict_New();
                    PyObject *matrix_dict = PyDict_New();
                    
                    auto it = make_move_iterator(dynamics.begin());
                    auto it_end = make_move_iterator(dynamics.end());
                    for (; it < it_end; it++) {
                      auto column_expr = static_cast<ComplexExpression>(*(it));
                      {
                        // head = <column_name>
                        auto [head, statics, dynamics, spans] =
                          move(column_expr).decompose();
                        auto column_name = head.getName().c_str();
                        auto it = std::make_move_iterator(dynamics.begin());
                        auto list_expr = static_cast<ComplexExpression>(*it);
                        {
                          // head = List
                          auto [head, statics, dynamics, spans] =
                            move(list_expr).decompose();
                          
                          py_list = spans_to_py_list(move(spans));
                          PyDict_SetItemString(table_dict, column_name, py_list);
                        }
                      }
                    }

                    PyDict_SetItemString(wrapper_dict, "table", table_dict);
                    PyDict_SetItemString(wrapper_dict, "matrix", matrix_dict);

                    PyDict_SetItemString(global_dict, table_name, wrapper_dict);
                  }
                }
              }

              PyObject *result =
                  PyRun_String(script, Py_file_input, global_dict, global_dict);

              if (result == nullptr) {
                PyErr_Print();
              } else {
                Py_DECREF(result);
              }
            }

            if (head == "get_python_var"_) {
              auto it = std::make_move_iterator(statics.begin());
              auto var_name = static_cast<Symbol>(*it).getName().c_str();

              auto wrapper_dict = PyDict_GetItemString(global_dict, var_name);
              auto table_dict = PyDict_GetItemString(wrapper_dict, "table");

              ExpressionArguments dynamics(PyDict_Size(table_dict));

              PyObject *col_name, *col_py_list;
              Py_ssize_t pos = 0;

              while (PyDict_Next(table_dict, &pos, &col_name, &col_py_list)) {
                ComplexExpression *boss_column;
                {
                  // head = <column-name>
                  Symbol head(move(*col_name));
                  auto spans = py_list_to_spans(col_py_list);
                  auto boss_list = ComplexExpression("List"_, {}, {}, move(spans));

                  ExpressionArguments dynamics{boss_list};
                  *boss_column = ComplexExpression(move(head), {}, {dynamics}, {});
                }
                dynamics.emplace_back(move(*boss_column));
              }

              auto result = ComplexExpression("Table"_, {}, {dynamics}, {});
              return result;
            }

            transform(make_move_iterator(dynamics.begin()),
                      make_move_iterator(dynamics.end()), dynamics.begin(),
                      [this](auto &&arg) {
                        return evaluate(forward<decltype(arg)>(arg));
                      });

            return boss::ComplexExpression(move(head), {}, move(dynamics),
                                           move(spans));
          },
          [this](Symbol &&symbol) -> boss::Expression {
            auto name = symbol.getName();
            cout << "symbol " << name << endl;

            return forward<decltype(symbol)>(symbol);
          },
          [](auto &&arg) -> boss::Expression {
            cout << "other type " << typeid(arg).name() << endl;

            return forward<decltype(arg)>(arg);
          }),
      forward<decltype(e)>(e));
};

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

Engine::Engine() {
  init_python_and_numpy();
  PyDict_SetItemString(global_dict, "__builtins__", PyEval_GetBuiltins());
}

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
