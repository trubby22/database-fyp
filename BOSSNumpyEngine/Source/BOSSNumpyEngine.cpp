#include "BOSSNumpyEngine.hpp"

#pragma region using

#pragma endregion using

// #define DEBUG

namespace boss::engines::numpy {

template <typename T>
Span<T> *transfer_ownership(Span<T> &&span) {
  return new Span<T>(move(span));
}

extern "C" {
  void print_destroy_msg() {
    // assert(false);
    // printf("destroying span...\n");
  }

  void destroy_span_int32(PyObject *capsule) {
    print_destroy_msg();
    void *span_ptr = PyCapsule_GetPointer(capsule, PyCapsule_GetName(capsule));
    delete static_cast<Span<int32_t>*>(span_ptr);
  };

  void destroy_span_int64(PyObject *capsule) {
    print_destroy_msg();
    void *span_ptr = PyCapsule_GetPointer(capsule, PyCapsule_GetName(capsule));
    delete static_cast<Span<int64_t>*>(span_ptr);
  };

  void destroy_span_float(PyObject *capsule) {
    print_destroy_msg();
    void *span_ptr = PyCapsule_GetPointer(capsule, PyCapsule_GetName(capsule));
    delete static_cast<Span<float_t>*>(span_ptr);
  };

  void destroy_span_double(PyObject *capsule) {
    print_destroy_msg();
    void *span_ptr = PyCapsule_GetPointer(capsule, PyCapsule_GetName(capsule));
    delete static_cast<Span<double_t>*>(span_ptr);
  };
}

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
    auto npy_arr = PyList_GetItem(list, i);
    Py_INCREF(npy_arr);
    print_1d_numpy_array(reinterpret_cast<PyArrayObject *>(npy_arr));
    Py_DECREF(npy_arr);
  }
  cout << "end of py list" << endl;
}

PythonExpressionSystem::ExpressionSpanArgument print_span_arg(PythonExpressionSystem::ExpressionSpanArgument &&arg) {
  return visit(
      []<typename T>(Span<T> &&typed_span) -> PythonExpressionSystem::ExpressionSpanArgument {
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

PythonExpressionSystem::ExpressionSpanArguments print_span_args(PythonExpressionSystem::ExpressionSpanArguments &&args) {
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

int sizeof_dtype(PyArrayObject *npy_arr) {
  int typenum = PyArray_TYPE(npy_arr);

  switch (typenum) {
    case NPY_INT32:
      return NPY_SIZEOF_INT;
      break;
    case NPY_INT64:
      return NPY_SIZEOF_LONG;
      break;
    case NPY_FLOAT:
      return NPY_SIZEOF_FLOAT;
      break;
    case NPY_DOUBLE:
      return NPY_SIZEOF_DOUBLE;
      break;
    default:
      throw runtime_error("shouldn't happen");
      break;
  }
}

#pragma endregion type_conversion

#pragma region python_to_boss

template <typename T> Span<T> numpy_arr_to_span_helper(PyObject *py_npy_arr) {
  auto npy_arr = reinterpret_cast<PyArrayObject *>(py_npy_arr);
  T *data = static_cast<T *>(PyArray_DATA(npy_arr));
  auto length = PyArray_SIZE(npy_arr);
  auto span = boss::Span<T>(data, length, [npy_arr]() {
    // cout << "deleting span" << endl;
    Py_DECREF(reinterpret_cast<PyObject *>(npy_arr));
  });
  return span;
}

PythonExpressionSystem::ExpressionSpanArgument Engine::numpy_arr_to_span(PyObject *npy_arr) {
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

PythonExpressionSystem::ExpressionSpanArguments Engine::npy_arr_to_spans(PyObject *npy_arr) {
  PythonExpressionSystem::ExpressionSpanArguments result;
  result.reserve(1);
  auto span_arg = numpy_arr_to_span(npy_arr);
  result.emplace_back(move(span_arg));
  return result;
}

PythonExpressionSystem::ExpressionSpanArguments Engine::py_list_to_spans(PyObject *list) {
  PythonExpressionSystem::ExpressionSpanArguments result;
  auto size = PyList_Size(list);
  result.reserve(size);
  for (int i = 0; i < size; i++) {
    auto npy_arr = PyList_GetItem(list, i);
    Py_INCREF(npy_arr);
    auto span_arg = numpy_arr_to_span(npy_arr);
    result.emplace_back(move(span_arg));
  }
  return result;
}

template <typename T>
PythonExpressionSystem::ComplexExpression Engine::npy_matrix_to_table_helper(PyArrayObject *npy_matrix,
                                                     PyObject *col_names) {
  Py_ssize_t col_names_size = PyList_Size(col_names);
  npy_intp *dims = PyArray_DIMS(npy_matrix);
  auto npy_rows = static_cast<int>(*dims);
  auto npy_cols = static_cast<ull>(*(dims + 1));
  assert(col_names_size == npy_rows);
  T *matrix_begin = static_cast<T *>(PyArray_DATA(npy_matrix));
  ull dtype_size = static_cast<ull>(sizeof_dtype(npy_matrix));
  ull boss_col_size_bytes = npy_cols * dtype_size;
  ull num_spans_per_boss_col = boss_col_size_bytes / span_size_bytes;
  ull mod = boss_col_size_bytes % span_size_bytes;
  if (mod > 0) {
    num_spans_per_boss_col += 1;
  }

  PythonExpressionSystem::ExpressionArguments res_dynamics;
  res_dynamics.reserve(npy_rows);

  for (int i = 0; i < npy_rows; i++) {
    auto col_name = PyList_GetItem(col_names, i);
    Py_INCREF(col_name);
    string col_name_str = PyObject_to_string(col_name);
    Py_DECREF(col_name);
    Symbol col_head(move(col_name_str));

    PythonExpressionSystem::ExpressionSpanArguments col_list_spans;
    col_list_spans.reserve(num_spans_per_boss_col);
    for (ull j = 0; j < npy_cols; j += span_size_bytes) {
      T *span_begin =
          matrix_begin + i * npy_cols + j;
      T *span_end = min(span_begin + span_size_bytes,
                        matrix_begin + (i + 1) * npy_cols);
#ifdef DEBUG
      // cout << "creating new boss span of size ";
      // cout << distance(span_begin, span_end) << endl;
      // cout << "npy_rows " << npy_rows << endl;
      cout << "num_spans_per_boss_col " << num_spans_per_boss_col << endl;
      cout << "npy_cols " << npy_cols << endl;
      cout << "boss_col_size_bytes " << boss_col_size_bytes << endl;
      cout << "span_size_bytes " << span_size_bytes << endl;
      cout << endl;
#endif
      vector<T> v;
      v.assign(move(span_begin), move(span_end));
      auto result = Span<T>(move(v));
      col_list_spans.emplace_back(move(result));
    }
    // auto boss_list = PythonExpressionSystem::ComplexExpression("List"_, {}, {}, move(col_list_spans));
    // PythonExpressionSystem::PythonExpressionSystem::ComplexExpression 
    auto boss_list = PythonExpressionSystem::ComplexExpression("List"_, {}, {}, move(col_list_spans));
    // head = List

    PythonExpressionSystem::ExpressionArguments col_dynamics;
    col_dynamics.reserve(1);
    col_dynamics.emplace_back(move(boss_list));

    auto boss_column =
        PythonExpressionSystem::ComplexExpression(move(col_head), {}, move(col_dynamics), {});
    // head = <col_name>

    res_dynamics.emplace_back(move(boss_column));
  }
  auto table = PythonExpressionSystem::ComplexExpression("Table"_, {}, move(res_dynamics), {});
  // table.steal_ref(reinterpret_cast<PyObject *>(npy_matrix));
  return table;
}

#pragma endregion python_to_boss

#pragma region boss_to_python

// returns new reference
PyObject *
Engine::span_to_numpy_arr(PythonExpressionSystem::ExpressionSpanArgument &&arg) {
  PyObject *result;
  visit(
      [&result]<typename T>(Span<T> &&typed_span) -> void {
        if constexpr (is_same_v<T, int32_t> || is_same_v<T, int64_t> ||
                      is_same_v<T, float_t> || is_same_v<T, double_t>) {

          auto typenum = cpp_type_to_numpy<T>();
          auto begin = typed_span.begin();
          auto size = typed_span.size();
          npy_intp dims[] = {static_cast<npy_intp>(size)};

          result = PyArray_SimpleNewFromData(1, dims, typenum, begin);

          void (*destroy_span)(PyObject *capsule_ptr);
          if constexpr (is_same_v<T, int32_t>) {
            destroy_span = &destroy_span_int32;
          } else if constexpr (is_same_v<T, int64_t>) {
            destroy_span = &destroy_span_int64;
          } else if constexpr (is_same_v<T, float_t>) {
            destroy_span = &destroy_span_float;
          } else if constexpr (is_same_v<T, double_t>) {
            destroy_span = &destroy_span_double;
          } else {
            throw runtime_error("unsupported type: " + string(typeid(T).name()));
          }

          Span<T> *span_ptr = transfer_ownership(move(typed_span));
          PyObject *capsule = PyCapsule_New(span_ptr, "backing_span",
                                  (PyCapsule_Destructor)destroy_span);
          if (PyArray_SetBaseObject(reinterpret_cast<PyArrayObject *>(result), capsule) == -1) {
            Py_DECREF(result);
            PyErr_Print();
            throw runtime_error("can't convert span to npy_arr - problems /w capsule");
          }
        } else {
          throw runtime_error("unsupported span type: " +
                              string(typeid(decltype(typed_span)).name()));
        }
      },
      forward<decltype(arg)>(arg));
  return result;
}

PyObject *
Engine::spans_to_py_list(PythonExpressionSystem::ExpressionSpanArguments &&args) {
  PyObject *result = PyList_New(args.size());
  auto it = make_move_iterator(args.begin());
  auto it_end = make_move_iterator(args.end());
  Py_ssize_t i = 0;
  for (; it < it_end; it += 1) {
    auto numpy_arr = span_to_numpy_arr(*it);
    PyList_SET_ITEM(result, i, numpy_arr);
    i++;
  }
  return result;
}

#pragma endregion boss_to_python

#pragma region conversion_new

// returns new reference to dict
PyObject *
Engine::table_to_pydict_column(PythonExpressionSystem::ComplexExpression &&table_expr) {
  PyObject *table_dict = PyDict_New();

  auto [table_unused_0, table_unused_1, table_dynamics,
        table_unused_3] = move(table_expr).decompose();
  // head = Table
  auto table_it = make_move_iterator(table_dynamics.begin());
  auto table_it_end = make_move_iterator(table_dynamics.end());
  for (; table_it < table_it_end; table_it++) {
    auto table_column_expr =
        get<PythonExpressionSystem::ComplexExpression>(*(table_it));

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
        get<PythonExpressionSystem::ComplexExpression>(*colname_it);

    auto [list_head, list_unused_1, list_unused_2, list_spans] =
        move(colname_list_expr).decompose();
    // head = List
    auto list_it = make_move_iterator(list_spans.begin());
    PyObject *numpy_arr = span_to_numpy_arr(*list_it);
    PyDict_SetItemString(table_dict, colname_column_name,
                          numpy_arr);
    Py_DECREF(numpy_arr);
  }

  return table_dict;
}

// returns new reference to dict
PyObject *
Engine::table_to_pydict_spans(PythonExpressionSystem::ComplexExpression &&table_expr) {
  PyObject *table_dict = PyDict_New();

  // PythonExpressionSystem::Expression table_expr = std_move(table_expr_arg);
  // if (dynamic_cast<ComplexExpressionWrapper *>(&table_expr)) {
  // }

  auto [table_unused_0, table_unused_1, table_dynamics,
        table_unused_3] = move(table_expr).decompose();
  // head = Table
  auto table_it = make_move_iterator(table_dynamics.begin());
  auto table_it_end = make_move_iterator(table_dynamics.end());
  for (; table_it < table_it_end; table_it++) {
    auto table_column_expr =
        get<PythonExpressionSystem::ComplexExpression>(*(table_it));

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
        get<PythonExpressionSystem::ComplexExpression>(*colname_it);

    auto [list_head, list_unused_1, list_unused_2, list_spans] =
        move(colname_list_expr).decompose();
    // head = List

    auto list_py_list = spans_to_py_list(move(list_spans));
    PyDict_SetItemString(table_dict, colname_column_name,
                          list_py_list);
    Py_DECREF(list_py_list);
  }

  return table_dict;
}

// returns new reference to wrapper_dict
PyObject *
Engine::table_to_pywrapper(PythonExpressionSystem::ComplexExpression &&table_expr) {
  PyObject *wrapper_dict = PyDict_New();
  PyObject *matrix_dict = PyDict_New();
  PyObject *table_dict = table_to_pydict_spans(move(table_expr));

  PyDict_SetItemString(wrapper_dict, "table", table_dict);
  Py_DECREF(table_dict);
  PyDict_SetItemString(wrapper_dict, "matrix", matrix_dict);
  Py_DECREF(matrix_dict);

  return wrapper_dict;
}

// steals reference to table_dict
PythonExpressionSystem::Expression
Engine::pydict_column_to_table(PyObject *table_dict) {
  PythonExpressionSystem::ExpressionArguments res_dynamics;
  res_dynamics.reserve(PyDict_Size(table_dict));

  PyObject *col_name, *npy_arr;
  Py_ssize_t pos = 0;
  while (PyDict_Next(table_dict, &pos, &col_name, &npy_arr)) {
    Py_INCREF(col_name);
    Py_INCREF(npy_arr);
    string col_name_str = PyObject_to_string(col_name);
    Py_DECREF(col_name);
    Symbol col_head(move(col_name_str));

    auto col_list_spans = numpy_arr_to_spans(npy_arr);
    Py_DECREF(npy_arr);
    auto boss_list =
        PythonExpressionSystem::ComplexExpression("List"_, {}, {}, move(col_list_spans));
    // head = List

    PythonExpressionSystem::ExpressionArguments col_dynamics;
    col_dynamics.reserve(1);
    col_dynamics.emplace_back(move(boss_list));

    auto boss_column = PythonExpressionSystem::ComplexExpression(move(col_head), {},
                                          move(col_dynamics), {});
    // head = <col_name>

    res_dynamics.emplace_back(move(boss_column));
  }
  Py_DECREF(table_dict);

  return PythonExpressionSystem::ComplexExpression("Table"_, {}, move(res_dynamics), {});
}

// steals reference to table_dict
PythonExpressionSystem::Expression
Engine::pydict_spans_to_table(PyObject *table_dict) {
  PythonExpressionSystem::ExpressionArguments res_dynamics;
  res_dynamics.reserve(PyDict_Size(table_dict));

  PyObject *col_name, *col_py_list;
  Py_ssize_t pos = 0;
  while (PyDict_Next(table_dict, &pos, &col_name, &col_py_list)) {
    Py_INCREF(col_name);
    Py_INCREF(col_py_list);
    string col_name_str = PyObject_to_string(col_name);
    Py_DECREF(col_name);
    Symbol col_head(move(col_name_str));

    auto col_list_spans = py_list_to_spans(col_py_list);
    Py_DECREF(col_py_list);
    auto boss_list =
        PythonExpressionSystem::ComplexExpression("List"_, {}, {}, move(col_list_spans));
    // head = List

    PythonExpressionSystem::ExpressionArguments col_dynamics;
    col_dynamics.reserve(1);
    col_dynamics.emplace_back(move(boss_list));

    auto boss_column = PythonExpressionSystem::ComplexExpression(move(col_head), {},
                                          move(col_dynamics), {});
    // head = <col_name>

    res_dynamics.emplace_back(move(boss_column));
  }
  Py_DECREF(table_dict);

  return PythonExpressionSystem::ComplexExpression("Table"_, {}, move(res_dynamics), {});
}

// steals reference to matrix_dict
PythonExpressionSystem::ComplexExpression Engine::pymatrix_to_table(PyObject *matrix_dict) {
  auto matrix = PyDict_GetItemString(matrix_dict, "data");
  Py_INCREF(matrix);
  auto npy_matrix = reinterpret_cast<PyArrayObject *>(matrix);
  auto col_names = PyDict_GetItemString(matrix_dict, "col_names");
  Py_INCREF(col_names);
  Py_DECREF(matrix_dict);
  int typenum = PyArray_TYPE(npy_matrix);

  PythonExpressionSystem::Expression result;
  switch (typenum) {
  case NPY_INT32:
    result = npy_matrix_to_table_helper<int32_t>(npy_matrix, col_names);
    break;
  case NPY_INT64:
    result = npy_matrix_to_table_helper<int64_t>(npy_matrix, col_names);
    break;
  case NPY_FLOAT:
    result = npy_matrix_to_table_helper<float_t>(npy_matrix, col_names);
    break;
  case NPY_DOUBLE:
    result = npy_matrix_to_table_helper<double_t>(npy_matrix, col_names);
    break;
  default:
    throw runtime_error("shouldn't happen");
    break;
  }

  Py_DECREF(col_names);
  Py_DECREF(matrix);
  return get<PythonExpressionSystem::ComplexExpression>(move(result));
}

// steals reference to wrapper_dict
PythonExpressionSystem::Expression
Engine::pywrapper_to_table(PyObject *wrapper_dict) {
  auto table_dict = PyDict_GetItemString(wrapper_dict, "table");
  Py_INCREF(table_dict);
  auto matrix_dict = PyDict_GetItemString(wrapper_dict, "matrix");
  Py_INCREF(matrix_dict);

  PythonExpressionSystem::Expression table;
  if (table_dict != Py_None) {
    table = pydict_spans_to_table(table_dict);
  } else {
    table = pymatrix_to_table(matrix_dict);
  }
  Py_DECREF(wrapper_dict);

  return table;
}

template <typename T>
PyObject *primitive_to_pyobject(T &&arg) {
  if constexpr (is_same_v<T, int64_t>) {
    return PyLong_FromLong(arg);
  } else if constexpr (is_same_v<T, double_t>) {
    return PyFloat_FromDouble(arg);
  } else if constexpr (is_same_v<T, string>) {
    return PyUnicode_FromString(arg.c_str());
  } else {
    throw runtime_error("unsupported type: " + string(typeid(T).name()));
  }
}

// returns new reference
PyObject *single_span_list_to_pylist(PythonExpressionSystem::ComplexExpression &&list) {
  auto [list_unused_0, list_unused_1, list_unused_2, list_spans] =
    forward<decltype(list)>(list).decompose();
  auto it = make_move_iterator(list_spans.begin());
  auto span_arg = *it;
  PyObject *py_list = PyList_New(span_arg.size());
  visit(
    []<typename T>(Span<T> &&typed_span) -> void {
      auto size = typed_span.size();
      for (int i = 0; i < size; i++) {
        PyObject *pyobject = primitive_to_pyobject<T>(move(typed_span[i]));
        PyList_SET_ITEM(py_list, i, pyobject);
        Py_DECREF(pyobject);
        i++;
      }
    },
    forward<decltype(span_arg)>(span_arg)
  );
  return py_list;
}

static boss::expressions::ExpressionSpanArgument toBOSSExpression(PythonExpressionSystem::ExpressionSpanArgument&& span) {
  return std::visit(
      []<typename T>(boss::Span<T>&& typedSpan) -> boss::expressions::ExpressionSpanArgument {
        return static_cast<boss::expressions::ExpressionSpanArgument>(std::move(typedSpan));
      },
      std::move(span));
}

static boss::Expression toBOSSExpression(PythonExpressionSystem::Expression&& expr) {
  return std::visit(
      boss::utilities::overload(
          [&](PythonExpressionSystem::ComplexExpression&& e) -> boss::Expression {
            auto [head, unused_, dynamics, spans] = std::move(e).decompose();

            boss::ExpressionArguments bossDynamics;
            bossDynamics.reserve(dynamics.size());
            std::transform(std::make_move_iterator(dynamics.begin()),
                           std::make_move_iterator(dynamics.end()),
                           std::back_inserter(bossDynamics), [&](auto&& arg) {
                             return toBOSSExpression(std::forward<decltype(arg)>(arg));
                           });

            boss::expressions::ExpressionSpanArguments bossSpans;
            bossSpans.reserve(spans.size());
            std::transform(
                std::make_move_iterator(spans.begin()), std::make_move_iterator(spans.end()),
                std::back_inserter(bossSpans),
                [](auto&& span) { return toBOSSExpression(std::forward<decltype(span)>(span)); });
            return boss::ComplexExpression(std::move(head), {}, std::move(bossDynamics),
                                            std::move(bossSpans));
          },
          [&](PyObject *&&e) -> boss::Expression {
            auto table = pydict_column_to_table(move(e));
            return toBOSSExpression(move(table));
          },
          [](auto&& otherTypes) -> boss::Expression { return otherTypes; }),
      std::move(expr));
}

PyObject *
Engine::python_expression_to_pyobject(PythonExpressionSystem::Expression &&expr) {
  return std::visit(
    boss::utilities::overload(
        [&](PythonExpressionSystem::ComplexExpression&& e) -> PyObject * {
          return table_to_pydict_column(move(e));
        },
        [&](PyObject *&&e) -> PyObject * {
          return move(e);
        },
        [](auto&& otherTypes) -> PyObject * { return otherTypes; }),
    std::move(expr));
}

#pragma endregion conversion_new

PythonExpressionSystem::Expression Engine::evaluate(PythonExpressionSystem::Expression &&e) {
  return visit(
      boss::utilities::overload(
          [this](PythonExpressionSystem::ComplexExpression &&expression) -> PythonExpressionSystem::Expression {

            // top-level
            auto [top_head, top_statics, top_dynamics, top_spans] =
                forward<decltype(expression)>(expression).decompose();

            if (top_head == "to_boss"_) {
              // head = to_boss
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::Expression>(*top_it);
              auto result = get<PyObject *>(evaluate(move(expr)));
              return pydict_column_to_table(result);
            }

            if (top_head == "to_python"_) {
              // head = to_python
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::Expression>(*top_it);
              auto result = get<PythonExpressionSystem::ComplexExpression>(evaluate(move(expr)));
              return table_to_pydict_column(move(result));
              // return PythonExpressionSystem::ComplexExpression("python"_, {}, {}, {});
            }

            // def project(table, col_names)
            if (top_head == "project"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::Expression>(*top_it);
              auto col_names_expr = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1));

              PyObject *table_pydict = get<PyObject *>(evaluate(move(expr)));
              PyObject *col_names = single_span_list_to_pylist(move(key_col_names_expr));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "project");
              if (py_operator == NULL) {
                PyErr_Print();
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(py_operator, "OO", table_pydict, col_names);
                if (result == NULL) {
                  PyErr_Print();
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              Py_DECREF(table_pydict);
              Py_DECREF(col_names);

              return result;
            }

            // def select(table, key_col_names, boolean_ops, vals)
            if (top_head == "select"_) {
              // head = select
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::Expression>(*top_it);
              auto key_col_names_expr = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1));
              auto boolean_ops_expr = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 2));
              auto vals_expr = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 3));

              PyObject *table_pydict = get<PyObject *>(evaluate(move(expr)));
              PyObject *key_col_names = single_span_list_to_pylist(move(key_col_names_expr));
              PyObject *boolean_ops = single_span_list_to_pylist(move(boolean_ops_expr));
              PyObject *vals = single_span_list_to_pylist(move(vals_expr));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "select");
              if (py_operator == NULL) {
                PyErr_Print();
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOO", table_pydict, key_col_names, boolean_ops, vals);
                if (result == NULL) {
                  PyErr_Print();
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              Py_DECREF(table_pydict);
              Py_DECREF(key_col_names);
              Py_DECREF(boolean_ops);
              Py_DECREF(vals);

              return result;
            }

            // def equi_join(table_1, table_2, key_col_names_1, key_col_names_2)
            if (top_head == "equi_join"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr_1 = get<PythonExpressionSystem::Expression>(*top_it);
              auto expr_2 = get<PythonExpressionSystem::Expression>(*(top_it + 1));
              auto key_col_names_expr_1 = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 2));
              auto key_col_names_expr_2 = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 3));

              PyObject *table_pydict_1 = get<PyObject *>(evaluate(move(expr_1)));
              PyObject *table_pydict_2 = get<PyObject *>(evaluate(move(expr_2)));
              PyObject *key_col_names_1 = single_span_list_to_pylist(move(key_col_names_expr_1));
              PyObject *key_col_names_2 = single_span_list_to_pylist(move(key_col_names_expr_2));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "equi_join");
              if (py_operator == NULL) {
                PyErr_Print();
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOO", table_pydict_1, table_pydict_2, key_col_names_1, key_col_names_2);
                if (result == NULL) {
                  PyErr_Print();
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              Py_DECREF(table_pydict_1);
              Py_DECREF(table_pydict_2);
              Py_DECREF(key_col_names_1);
              Py_DECREF(key_col_names_2);

              return result;
            }

            // def aggregate(table, key_col_names, reduction_func, reduction_col_name)
            if (top_head == "aggregate"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::Expression>(*top_it);
              auto key_col_names_expr = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1));
              auto reduction_func_expr = get<Symbol>(*(top_it + 2));
              auto reduction_col_name_expr = get<Symbol>(*(top_it + 3));

              PyObject *table_pydict = get<PyObject *>(evaluate(move(expr)));
              PyObject *key_col_names = single_span_list_to_pylist(move(key_col_names_expr));
              string reduction_func_str = reduction_func_expr.getName();
              string reduction_col_name_str = reductino_col_name_expr.getName();
              PyObject *reduction_func = primitive_to_pyobject<string>(move(reduction_func_str));
              PyObject *reduction_func_col_name = primitive_to_pyobject<string>(move(reduction_func_col_name_str));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "aggregate");
              if (py_operator == NULL) {
                PyErr_Print();
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOO", table_pydict, key_col_names, reduction_func, reduction_func_col_name);
                if (result == NULL) {
                  PyErr_Print();
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              Py_DECREF(table_pydict);
              Py_DECREF(key_col_names);

              return result;
            }

            if (top_head == "python_globals"_) {
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto top_script_str = get<Symbol>(*top_it).getName();
              auto top_script = move(top_script_str).c_str();

              PyObject *top_result = PyRun_String(top_script, Py_file_input,
                                                  global_dict, global_dict);

              if (PyErr_Occurred()) {
                PyErr_Print();
                throw runtime_error("error in provided python code");
              }

              string top_script_return = top_script;
              *top_it = Symbol(move(top_script_return));

              if (top_result != nullptr) {
                Py_DECREF(top_result);
              }

              auto result =
                  PythonExpressionSystem::ComplexExpression(move(top_head), move(top_statics),
                                    move(top_dynamics), move(top_spans));

              return result;
            }

            if (top_head == "python"_) {
              // head = Python
              auto top_dynamics_size = top_dynamics.size();
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto top_script_str = get<Symbol>(*top_it).getName();
              auto top_script = move(top_script_str).c_str();

              if (top_dynamics_size >= 2) {
                auto top_where = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1));

                auto [where_unused_0, where_unused_1, where_dynamics,
                      where_unused_3] = move(top_where).decompose();
                // head = Where
                auto where_it = make_move_iterator(where_dynamics.begin());
                auto where_it_end = make_move_iterator(where_dynamics.end());
                for (; where_it < where_it_end; where_it += 2) {
                  auto where_table_name_str = get<Symbol>(*where_it).getName();
                  auto where_table_name = move(where_table_name_str).c_str();
                  auto where_table_expr =
                      get<PythonExpressionSystem::ComplexExpression>(*(where_it + 1));
                  PyObject *wrapper_dict = table_to_pywrapper(move(where_table_expr));
                  PyDict_SetItemString(local_dict, where_table_name,
                                       wrapper_dict);
                  Py_DECREF(wrapper_dict);
                }
              }

              PyObject *top_result = PyRun_String(top_script, Py_file_input,
                                                  global_dict, local_dict);
              if (PyErr_Occurred()) {
                PyErr_Print();
                throw runtime_error("error in provided python code");
              }
              if (top_result != nullptr) {
                Py_DECREF(top_result);
              }

              return PythonExpressionSystem::ComplexExpression("python"_, {}, {}, {});
            }

            if (top_head == "get_python_var"_) {
              // head = get_python_var
              auto it = make_move_iterator(top_dynamics.begin());
              auto var_name_str = get<Symbol>(*it).getName();
              auto var_name = move(var_name_str).c_str();

              auto wrapper_dict =
                  PyDict_GetItemString(local_dict, var_name);
              Py_INCREF(wrapper_dict);
              return pywrapper_to_table(wrapper_dict);
            }

            transform(make_move_iterator(top_dynamics.begin()),
                      make_move_iterator(top_dynamics.end()),
                      top_dynamics.begin(), [this](auto &&arg) {
                        auto result = evaluate(forward<decltype(arg)>(arg));
                        return get<PythonExpressionSystem::Expression>(move(result));
                      });

            return PythonExpressionSystem::ComplexExpression(move(top_head), {}, move(top_dynamics),
                                     move(top_spans));
          },
          [this](Symbol &&symbol) -> PythonExpressionSystem::Expression {
            auto name = symbol.getName();

            if (symbol == "reset_python_dict"_) {
              reset_python_dict();
            }

            return forward<decltype(symbol)>(symbol);
          },
          [](auto &&arg) -> PythonExpressionSystem::Expression { return forward<decltype(arg)>(arg); }),
      forward<decltype(e)>(e));
};

boss::expressions::Expression Engine::evaluate_c(boss::expressions::Expression &&e) {
  auto result = evaluate(move(e));
  return toBOSSExpression(move(result));
}

#pragma region boilerplate

void Engine::init_python_and_numpy() {
  Py_Initialize();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
  import_array();
#pragma clang diagnostic pop
  if (PyErr_Occurred()) {
    throw runtime_error("Failed to import numpy Python module(s).");
  }
  assert(PyArray_API);

  global_dict = PyDict_New();
  local_dict = PyDict_New();
  PyRun_String(R"(
import sys
sys.path.append("/mnt/ubuntu-image-repos/BOSSNumpyEngine/rel_alg")
import numpy as np
  )", Py_file_input, global_dict, local_dict);

  main_module = PyImport_AddModule("__main__");
  rel_alg = PyImport_ImportModule("rel_alg");
  if (rel_alg == NULL) {
    PyErr_Print();
    Py_Finalize();
    throw runtime_error("error when importing rel_alg module");
  }
}

void Engine::reset_python_dict() {
  // Py_ssize_t size = PyDict_Size(local_dict);
  // cout << "local_dict size " << size << endl;
  // Py_ssize_t ref_count = Py_REFCNT(local_dict);
  // cout << "local_dict reference count " << ref_count << endl;

  Py_DECREF(local_dict);
  local_dict = PyDict_New();
}

Engine::Engine(ull span_size_bytes) : span_size_bytes(span_size_bytes) {
  init_python_and_numpy();
  PyDict_SetItemString(global_dict, "__builtins__", PyEval_GetBuiltins());
}

Engine::~Engine() {
  Py_DECREF(local_dict);
}

#pragma endregion boilerplate

} // namespace boss::engines::numpy

#pragma region boilerplate

static auto &enginePtr(bool initialise = true) {
  static auto engine = unique_ptr<boss::engines::numpy::Engine>();
  if (!engine && initialise) {
    engine.reset(new boss::engines::numpy::Engine(ENGINE_SPAN_SIZE_BYTES));
  }
  return engine;
}

extern "C" BOSSExpression *evaluate(BOSSExpression *e) {
  static mutex m;
  lock_guard lock(m);
  auto *r = new BOSSExpression{enginePtr()->evaluate_c(move(e->delegate))};
  return r;
};

extern "C" void reset() { enginePtr(false).reset(nullptr); }

#pragma endregion boilerplate
