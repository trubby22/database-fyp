#include "BOSSNumpyEngine.hpp"

#pragma region using

using namespace std;
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

#pragma endregion using

namespace boss::engines::numpy {

#pragma region python_helpers

string PyObject_to_string(PyObject *obj) {
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  string result;

  if (PyUnicode_Check(obj)) {
    const char *c_str = PyUnicode_AsUTF8(obj);
    if (c_str) {
      result = string(c_str);
    }
  } else {
    PyObject *unicodeObj = PyUnicode_FromObject(obj);
    if (unicodeObj) {
      const char *c_str = PyUnicode_AsUTF8(unicodeObj);
      if (c_str) {
        result = string(c_str);
      }
      Py_DECREF(unicodeObj);
    }
  }

  PyGILState_Release(gstate);

  return result;
}

#pragma endregion python_helpers

#pragma region print

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

void print_py_list(PyObject *list) {
  cout << "printing py list" << endl;
  Py_ssize_t size = PyList_Size(list);
  for (Py_ssize_t i = 0; i < size; i++) {
    auto npy_arr = reinterpret_cast<PyArrayObject *>(PyList_GetItem(list, i));
    print_1d_numpy_array(npy_arr);
  }
  cout << "end of py list" << endl;
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

#pragma endregion print

#pragma region type_conversion

template <typename T> NPY_TYPES cpp_type_to_numpy() {
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

#pragma endregion type_conversion

#pragma region benchmark

template <class Generator>
Span<int> create_random_span(int size, Generator g) {
  vector<int> vec(size);
  generate(begin(vec), end(vec), g);

  auto result = Span<int>(move(vec));
  return result;
}

ComplexExpression create_random_table(int num_cols, int num_spans, int span_size) {
  random_device rnd_device;
  mt19937 mersenne_engine {rnd_device()};
  uniform_int_distribution<int> dist {0, 100};
  auto gen = [&dist, &mersenne_engine](){
                  return dist(mersenne_engine);
              };

  ExpressionArguments table_dynamics;
  for (int i = 0; i < num_cols; i++) {
    ExpressionSpanArguments list_spans;
    for (int j = 0; j < num_spans; j++) {
      auto span = create_random_span<function<int()> >(span_size, gen);
      list_spans.emplace_back(move(span));
    }
    auto list = ComplexExpression("List"_, {}, {}, move(list_spans));
    // List

    ExpressionArguments column_dynamics;
    column_dynamics.emplace_back(move(list));

    ostringstream oss;
    oss << "col_" << i;
    string col_name_str = oss.str();
    Symbol col_name(move(col_name_str));

    auto column = ComplexExpression(move(col_name), {}, move(column_dynamics));
    // Column
    table_dynamics.emplace_back(move(column));
  }

  return ComplexExpression("Table"_, {}, move(table_dynamics), {});
}

#pragma endregion benchmark

#pragma region python_to_boss

template <typename T> Span<T> numpy_arr_to_span_helper(PyArrayObject *npy_arr) {
  T *data = static_cast<T *>(PyArray_DATA(npy_arr));
  auto length = PyArray_SIZE(npy_arr);
  vector<T> v;
  v.assign(move(data), move(static_cast<T *>(data + length)));
  auto result = Span<T>(move(v));
  return result;
}

ExpressionSpanArgument numpy_arr_to_span(PyArrayObject *npy_arr) {
  int typenum = PyArray_TYPE(npy_arr);

  switch (typenum) {
  case NPY_INT32:
    return numpy_arr_to_span_helper<int32_t>(npy_arr);
    break;
  case NPY_INT64:
    return numpy_arr_to_span_helper<int64_t>(npy_arr);
    break;
  case NPY_FLOAT:
    return numpy_arr_to_span_helper<float_t>(npy_arr);
    break;
  case NPY_DOUBLE:
    return numpy_arr_to_span_helper<double_t>(npy_arr);
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
    auto npy_arr = reinterpret_cast<PyArrayObject *>(PyList_GetItem(list, i));
    auto span_arg = numpy_arr_to_span(npy_arr);
    result.emplace_back(move(span_arg));
  }
  return result;
}

template <typename T>
ComplexExpression Engine::npy_matrix_to_table_helper(PyArrayObject *npy_matrix,
                                                     PyObject *col_names) {

  cout << "npy_matrix_to_table_helper" << endl;
  Py_ssize_t col_names_size = PyList_Size(col_names);
  npy_intp *dims = PyArray_DIMS(npy_matrix);
  auto num_rows = *dims;
  auto num_cols = *(dims + 1);
  assert(col_names_size == num_rows);
  T *matrix_begin = static_cast<T *>(PyArray_DATA(npy_matrix));

  ExpressionArguments res_dynamics;
  res_dynamics.reserve(num_rows);

  for (int i = 0; i < num_rows; i++) {
    auto col_name = PyList_GetItem(col_names, i);
    string col_name_str = PyObject_to_string(col_name);
    Symbol col_head(move(col_name_str));

    ExpressionSpanArguments col_list_spans;
    int div = num_cols / span_size;
    int mod = num_cols % span_size;
    if (mod > 0) {
      div += 1;
    }
    col_list_spans.reserve(div);
    for (int j = 0; j < num_cols; j += span_size) {
      T *span_begin =
          matrix_begin + i * num_cols + j * span_size;
      T *span_end = min(span_begin + span_size,
                        matrix_begin + (i + 1) * num_cols);
      vector<T> v;
      v.assign(move(span_begin), move(span_end));
      auto result = Span<T>(move(v));
      col_list_spans.emplace_back(move(result));
    }

    auto boss_list = ComplexExpression("List"_, {}, {}, move(col_list_spans));
    // head = List

    ExpressionArguments col_dynamics;
    col_dynamics.reserve(1);
    col_dynamics.emplace_back(move(boss_list));

    auto boss_column =
        ComplexExpression(move(col_head), {}, move(col_dynamics), {});
    // head = <col_name>

    res_dynamics.emplace_back(move(boss_column));
  }
  auto table = ComplexExpression("Table"_, {}, move(res_dynamics), {});
  return table;
}

ComplexExpression Engine::npy_matrix_to_table(PyArrayObject *npy_matrix,
                                              PyObject *col_names) {
  int typenum = PyArray_TYPE(npy_matrix);

  switch (typenum) {
  case NPY_INT32:
    return npy_matrix_to_table_helper<int32_t>(npy_matrix, col_names);
    break;
  case NPY_INT64:
    return npy_matrix_to_table_helper<int64_t>(npy_matrix, col_names);
    break;
  case NPY_FLOAT:
    return npy_matrix_to_table_helper<float_t>(npy_matrix, col_names);
    break;
  case NPY_DOUBLE:
    return npy_matrix_to_table_helper<double_t>(npy_matrix, col_names);
    break;
  default:
    throw runtime_error("shouldn't happen");
    break;
  }
}

#pragma endregion python_to_boss

#pragma region boss_to_python

tuple<ExpressionSpanArgument, PyObject *>
span_to_numpy_arr(ExpressionSpanArgument &&arg) {
  PyObject *result;
  ExpressionSpanArgument result_arg = visit(
      [&result]<typename T>(Span<T> &&typed_span) -> ExpressionSpanArgument {
        if constexpr (is_same_v<T, int32_t> || is_same_v<T, int64_t> ||
                      is_same_v<T, float_t> || is_same_v<T, double_t>) {

          auto typenum = cpp_type_to_numpy<T>();
          auto begin = typed_span.begin();
          auto end = typed_span.end();
          auto size = typed_span.size();
          npy_intp dims[] = {static_cast<npy_intp>(size)};

          result = PyArray_SimpleNewFromData(1, dims, typenum, begin);

          return typed_span;
        } else {
          throw runtime_error("unsupported span type: " +
                              string(typeid(decltype(typed_span)).name()));
        }
      },
      forward<decltype(arg)>(arg));
  return make_tuple(move(result_arg), move(result));
}

tuple<ExpressionSpanArguments, PyObject *>
spans_to_py_list(ExpressionSpanArguments &&args) {
  PyObject *result = PyList_New(args.size());
  auto it = make_move_iterator(args.begin());
  auto it_end = make_move_iterator(args.end());
  Py_ssize_t i = 0;
  for (; it < it_end; it += 1) {
    auto t = span_to_numpy_arr(*it);
    *it = move(get<0>(t));
    auto numpy_arr = move(get<1>(t));
    PyList_SET_ITEM(result, i, numpy_arr);
    i++;
  }
  return make_tuple(move(args), move(result));
}

#pragma endregion boss_to_python

Expression Engine::evaluate(Expression &&e) {
  return visit(
      boss::utilities::overload(
          [this](ComplexExpression &&expression) -> Expression {

            if (expression.getHead() == "Table"_) {
              cout << expression << endl;
            }

            // top-level
            auto [top_head, top_statics, top_dynamics, top_spans] =
                forward<decltype(expression)>(expression).decompose();

            if (top_head == "Python"_) {
              // head = Python
              auto top_dynamics_size = top_dynamics.size();
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto top_script_str = get<Symbol>(*top_it).getName();
              auto top_script = move(top_script_str).c_str();

              if (top_dynamics_size >= 2) {
                auto top_where = get<ComplexExpression>(*(top_it + 1));

                auto [where_unused_0, where_unused_1, where_dynamics,
                      where_unused_3] = move(top_where).decompose();
                // head = Where
                auto where_it = make_move_iterator(where_dynamics.begin());
                auto where_it_end = make_move_iterator(where_dynamics.end());
                for (; where_it < where_it_end; where_it += 2) {
                  auto where_table_name_str = get<Symbol>(*where_it).getName();
                  auto where_table_name = move(where_table_name_str).c_str();
                  auto where_table_expr =
                      get<ComplexExpression>(*(where_it + 1));

                  auto [table_unused_0, table_unused_1, table_dynamics,
                        table_unused_3] = move(where_table_expr).decompose();
                  // head = Table

                  PyObject *wrapper_dict = PyDict_New();
                  PyObject *table_dict = PyDict_New();
                  PyObject *matrix_dict = PyDict_New();

                  auto table_it = make_move_iterator(table_dynamics.begin());
                  auto table_it_end = make_move_iterator(table_dynamics.end());
                  for (; table_it < table_it_end; table_it++) {
                    auto table_column_expr =
                        get<ComplexExpression>(*(table_it));

                    auto [colname_head, colname_unused_1, colname_dynamics,
                          colname_unused_3] =
                        move(table_column_expr).decompose();
                    // head = <column_name>
                    auto colname_column_name_str = colname_head.getName();
                    auto colname_column_name =
                        move(colname_column_name_str).c_str();
                    auto colname_it =
                        make_move_iterator(colname_dynamics.begin());
                    auto colname_list_expr =
                        get<ComplexExpression>(*colname_it);

                    auto [list_head, list_unused_1, list_unused_2, list_spans] =
                        move(colname_list_expr).decompose();
                    // head = List

                    auto t = spans_to_py_list(move(list_spans));
                    list_spans = move(get<0>(t));
                    auto list_py_list = move(get<1>(t));
                    PyDict_SetItemString(table_dict, colname_column_name,
                                         move(list_py_list));

                    auto return_list =
                        ComplexExpression("List"_, {}, {}, move(list_spans));

                    *colname_it = move(return_list);
                    string colname_column_name_str_return = colname_column_name;
                    Symbol colname_column_name_return =
                        Symbol(move(colname_column_name));
                    auto return_col =
                        ComplexExpression(move(colname_column_name_return), {},
                                          move(colname_dynamics), {});

                    *table_it = move(return_col);
                  }

                  auto return_table =
                      ComplexExpression("Table"_, {}, move(table_dynamics), {});

                  *(where_it + 1) = move(return_table);

                  PyDict_SetItemString(wrapper_dict, "table", table_dict);
                  PyDict_SetItemString(wrapper_dict, "matrix", matrix_dict);
                  PyDict_SetItemString(global_dict, where_table_name,
                                       wrapper_dict);

                  string where_table_name_str_return = where_table_name;
                  *where_it = Symbol(move(where_table_name_str_return));
                }

                auto return_where =
                    ComplexExpression("Where"_, {}, move(where_dynamics), {});

                *(top_it + 1) = move(return_where);
              }

              PyObject *top_result = PyRun_String(top_script, Py_file_input,
                                                  global_dict, global_dict);

              string top_script_return = top_script;
              *top_it = Symbol(move(top_script_return));

              if (top_result == nullptr) {
                PyErr_Print();
              } else {
                Py_DECREF(top_result);
              }

              auto result =
                  ComplexExpression(move(top_head), move(top_statics),
                                    move(top_dynamics), move(top_spans));

              return result;
            }

            if (top_head == "get_python_var"_) {
              // head = get_python_var
              auto it = make_move_iterator(top_dynamics.begin());
              auto var_name_str = get<Symbol>(*it).getName();
              auto var_name = move(var_name_str).c_str();

              auto wrapper_dict =
                  PyDict_GetItemString(global_dict, move(var_name));
              auto table_dict = PyDict_GetItemString(wrapper_dict, "table");
              auto matrix_dict = PyDict_GetItemString(wrapper_dict, "matrix");

              Py_ssize_t table_dict_size = PyDict_Size(table_dict);
              if (table_dict_size > 0) {
                // boss table
                ExpressionArguments res_dynamics;
                res_dynamics.reserve(PyDict_Size(table_dict));

                PyObject *col_name, *col_py_list;
                Py_ssize_t pos = 0;

                while (PyDict_Next(table_dict, &pos, &col_name, &col_py_list)) {
                  string col_name_str = PyObject_to_string(col_name);
                  Symbol col_head(move(col_name_str));

                  auto col_list_spans = py_list_to_spans(col_py_list);
                  auto boss_list =
                      ComplexExpression("List"_, {}, {}, move(col_list_spans));
                  // head = List

                  ExpressionArguments col_dynamics;
                  col_dynamics.reserve(1);
                  col_dynamics.emplace_back(move(boss_list));

                  auto boss_column = ComplexExpression(move(col_head), {},
                                                       move(col_dynamics), {});
                  // head = <col_name>

                  res_dynamics.emplace_back(move(boss_column));
                }

                auto table =
                    ComplexExpression("Table"_, {}, move(res_dynamics), {});

                return table;
              } else {
                // matrix
                auto matrix = PyDict_GetItemString(matrix_dict, "data");
                auto col_names = PyDict_GetItemString(matrix_dict, "col_names");
                auto table = npy_matrix_to_table(
                    reinterpret_cast<PyArrayObject *>(matrix), col_names);
                return table;
              }
            }

            transform(make_move_iterator(top_dynamics.begin()),
                      make_move_iterator(top_dynamics.end()),
                      top_dynamics.begin(), [this](auto &&arg) {
                        return evaluate(forward<decltype(arg)>(arg));
                      });

            return ComplexExpression(move(top_head), {}, move(top_dynamics),
                                     move(top_spans));
          },
          [this](Symbol &&symbol) -> Expression {
            auto name = symbol.getName();

            return forward<decltype(symbol)>(symbol);
          },
          [](auto &&arg) -> Expression { return forward<decltype(arg)>(arg); }),
      forward<decltype(e)>(e));
};

#pragma region boilerplate

void Engine::init_python_and_numpy() {
  Py_Initialize();
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\".\")");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
  import_array();
#pragma clang diagnostic pop
  if (PyErr_Occurred()) {
    throw runtime_error("Failed to import numpy Python module(s).");
  }
  assert(PyArray_API);

  global_dict = PyDict_New();
}

Engine::Engine() : span_size(1 << 20) {
  init_python_and_numpy();
  PyDict_SetItemString(global_dict, "__builtins__", PyEval_GetBuiltins());
}

#pragma endregion boilerplate

} // namespace boss::engines::numpy

#pragma region boilerplate

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

#pragma endregion boilerplate
