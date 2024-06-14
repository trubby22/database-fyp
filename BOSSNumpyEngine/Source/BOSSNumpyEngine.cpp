#include "BOSSNumpyEngine.hpp"

#pragma region using

using ExpressionBuilder = boss::utilities::ExtensibleExpressionBuilder<PythonExpressionSystem>;
static ExpressionBuilder operator""_(const char* name, size_t /*unused*/) {
  return ExpressionBuilder(name);
};

#pragma endregion using

// #define DEBUG

namespace boss::engines::numpy {

void print_pylist(PyObject* pylist) {
    if (pylist && PyList_Check(pylist)) {
        Py_ssize_t size = PyList_Size(pylist);
        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GetItem(pylist, i);  // Borrowed reference
            PyObject* item_str = PyObject_Str(item);     // New reference

            if (item_str) {
                const char* item_cstr = PyUnicode_AsUTF8(item_str);
                if (item_cstr) {
                    std::cout << "Item " << i << ": " << item_cstr << std::endl;
                } else {
                    std::cerr << "Failed to convert item " << i << " to string." << std::endl;
                }
                // Py_DECREF(item_str);  // Decrement reference count of item_str
            } else {
                std::cerr << "Failed to get string representation of item " << i << std::endl;
            }
        }
    } else {
        std::cerr << "The provided object is not a list." << std::endl;
    }
}

template <typename T>
Span<T> *Engine::transfer_ownership(Span<T> &&span) {
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

string Engine::PyObject_to_string(PyObject *obj) {
  // PyGILState_STATE gstate;
  // gstate = PyGILState_Ensure();

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
      // Py_DECREF(unicodeObj);
    }
  }

  // PyGILState_Release(gstate);

  return result;
}

#pragma endregion python_helpers

#pragma region print

template <typename T> void Engine::print_1d_numpy_array_helper(PyArrayObject *array) {
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

void Engine::print_1d_numpy_array(PyArrayObject *array) {
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

void Engine::print_py_list(PyObject *list) {
  cout << "printing py list" << endl;
  Py_ssize_t size = PyList_Size(list);
  for (Py_ssize_t i = 0; i < size; i++) {
    auto npy_arr = PyList_GetItem(list, i);
    // Py_INCREF(npy_arr);
    print_1d_numpy_array(reinterpret_cast<PyArrayObject *>(npy_arr));
    // Py_DECREF(npy_arr);
  }
  cout << "end of py list" << endl;
}

PythonExpressionSystem::ExpressionSpanArgument Engine::print_span_arg(PythonExpressionSystem::ExpressionSpanArgument &&arg) {
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

PythonExpressionSystem::ExpressionSpanArguments Engine::print_span_args(PythonExpressionSystem::ExpressionSpanArguments &&args) {
  cout << "span args" << endl;
  cout << "[";

  transform(make_move_iterator(args.begin()), make_move_iterator(args.end()),
            args.begin(), [this](auto &&arg) {
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

template <typename T> NPY_TYPES Engine::cpp_type_to_numpy() {
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

int Engine::sizeof_dtype(PyArrayObject *npy_arr) {
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

template <typename T> Span<T> Engine::numpy_arr_to_span_helper(PyObject *py_npy_arr) {
  auto npy_arr = reinterpret_cast<PyArrayObject *>(py_npy_arr);
  T *data = static_cast<T *>(PyArray_DATA(npy_arr));
  auto length = PyArray_SIZE(npy_arr);
  auto span = boss::Span<T>(data, length, [
    // npy_arr
    ]() {
    // cout << "deleting span" << endl;
    // Py_DECREF(reinterpret_cast<PyObject *>(npy_arr));
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

template <typename T>
PythonExpressionSystem::ExpressionSpanArguments Engine::numpy_arr_to_spans_spans_helper(PyArrayObject *npy_arr) {
  npy_intp *dims = PyArray_DIMS(npy_arr);
  auto num_elems_in_npy_row = static_cast<ull>(*(dims));

  T *arr_begin = static_cast<T *>(PyArray_DATA(npy_arr));
  ull dtype_size = static_cast<ull>(sizeof_dtype(npy_arr));

  ull num_elems_per_full_span = span_size_bytes / dtype_size;
  ull num_full_spans_per_boss_col = num_elems_in_npy_row / num_elems_per_full_span;
  ull num_elems_in_last_non_full_span = num_elems_in_npy_row % num_elems_per_full_span;

  ull total_num_spans = num_full_spans_per_boss_col;
  if (num_elems_in_last_non_full_span > 0) {
    total_num_spans += 1;
  }

  PythonExpressionSystem::ExpressionSpanArguments col_list_spans;
  col_list_spans.reserve(total_num_spans);

  for (ull j = 0; j < total_num_spans; j++) {
    T *span_begin = arr_begin + j * num_elems_per_full_span;
    vector<T> v;
    ull num_elems_in_cur_span;
    if (j < num_full_spans_per_boss_col) {
      num_elems_in_cur_span = num_elems_per_full_span;
    } else {
      num_elems_in_cur_span = num_elems_in_last_non_full_span;
    }

    // Py_INCREF(reinterpret_cast<PyObject *>(npy_arr));
    auto span = boss::Span<T>(span_begin, num_elems_in_cur_span, [
      // npy_arr
      ]() {
      // cout << "deleting materialised column view" << endl;
      // Py_DECREF(reinterpret_cast<PyObject *>(npy_arr));
    });
    col_list_spans.emplace_back(move(span));
  }

  return col_list_spans;
}

PythonExpressionSystem::ExpressionSpanArguments Engine::numpy_arr_to_spans_spans(PyArrayObject *npy_arr) {
  int typenum = PyArray_TYPE(npy_arr);

  switch (typenum) {
  case NPY_INT32:
    return numpy_arr_to_spans_spans_helper<int32_t>(npy_arr);
    break;
  case NPY_INT64:
    return numpy_arr_to_spans_spans_helper<int64_t>(npy_arr);
    break;
  case NPY_FLOAT:
    return numpy_arr_to_spans_spans_helper<float_t>(npy_arr);
    break;
  case NPY_DOUBLE:
    return numpy_arr_to_spans_spans_helper<double_t>(npy_arr);
    break;
  default:
    throw runtime_error("shouldn't happen");
    break;
  }
}

PythonExpressionSystem::ExpressionSpanArguments Engine::numpy_arr_to_column_spans(PyObject *npy_arr) {
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
    // Py_INCREF(npy_arr);
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
  auto num_npy_rows = static_cast<int>(*dims);
  auto num_elems_in_npy_row = static_cast<ull>(*(dims + 1));
  assert(col_names_size == num_npy_rows);
  T *matrix_begin = static_cast<T *>(PyArray_DATA(npy_matrix));
  ull dtype_size = static_cast<ull>(sizeof_dtype(npy_matrix));

  ull num_elems_per_full_span = span_size_bytes / dtype_size;
  ull num_full_spans_per_boss_col = num_elems_in_npy_row / num_elems_per_full_span;
  ull num_elems_in_last_non_full_span = num_elems_in_npy_row % num_elems_per_full_span;

  ull total_num_spans = num_full_spans_per_boss_col;
  if (num_elems_in_last_non_full_span > 0) {
    total_num_spans += 1;
  }

  PythonExpressionSystem::ExpressionArguments res_dynamics;
  res_dynamics.reserve(num_npy_rows);

  for (int i = 0; i < num_npy_rows; i++) {
    auto col_name = PyList_GetItem(col_names, i);
    // Py_INCREF(col_name);
    string col_name_str = PyObject_to_string(col_name);
    // Py_DECREF(col_name);
    Symbol col_head(move(col_name_str));

    PythonExpressionSystem::ExpressionSpanArguments col_list_spans;
    col_list_spans.reserve(total_num_spans);

    for (ull j = 0; j < total_num_spans; j++) {
      T *span_begin = matrix_begin + i * num_elems_in_npy_row + j * num_elems_per_full_span;
      vector<T> v;
      ull num_elems_in_cur_span;
      if (j < num_full_spans_per_boss_col) {
        num_elems_in_cur_span = num_elems_per_full_span;
      } else {
        num_elems_in_cur_span = num_elems_in_last_non_full_span;
      }

      // Py_INCREF(reinterpret_cast<PyObject *>(npy_matrix));
      auto span = boss::Span<T>(span_begin, num_elems_in_cur_span, [
        // npy_matrix
        ]() {
        // cout << "deleting matrix view" << endl;
        // Py_DECREF(reinterpret_cast<PyObject *>(npy_matrix));
      });
      col_list_spans.emplace_back(move(span));
    }

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
      [&result, this]<typename T>(Span<T> &&typed_span) -> void {
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
            // Py_DECREF(result);
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
    // Py_DECREF(numpy_arr);
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
    // Py_DECREF(list_py_list);
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
  // Py_DECREF(table_dict);
  PyDict_SetItemString(wrapper_dict, "matrix", matrix_dict);
  // Py_DECREF(matrix_dict);

  return wrapper_dict;
}

// steals reference to table_dict
PythonExpressionSystem::Expression
Engine::pydict_column_to_table_column(PyObject *table_dict) {
  PythonExpressionSystem::ExpressionArguments res_dynamics;
  res_dynamics.reserve(PyDict_Size(table_dict));

  PyObject *col_name, *npy_arr;
  Py_ssize_t pos = 0;
  while (PyDict_Next(table_dict, &pos, &col_name, &npy_arr)) {
    // Py_INCREF(col_name);
    // Py_INCREF(npy_arr);
    string col_name_str = PyObject_to_string(col_name);
    // Py_DECREF(col_name);
    Symbol col_head(move(col_name_str));

    auto col_list_spans = numpy_arr_to_column_spans(npy_arr);
    // Py_DECREF(npy_arr);
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
  // Py_DECREF(table_dict);

  return PythonExpressionSystem::ComplexExpression("Table"_, {}, move(res_dynamics), {});
}

// steals reference to table_dict
PythonExpressionSystem::Expression
Engine::pydict_col_or_spans_to_table_spans(PyObject *table_dict) {
  PythonExpressionSystem::ExpressionArguments res_dynamics;
  res_dynamics.reserve(PyDict_Size(table_dict));

  PyObject *col_name, *col_pylist_or_npy_arr;
  Py_ssize_t pos = 0;
  while (PyDict_Next(table_dict, &pos, &col_name, &col_pylist_or_npy_arr)) {
    // Py_INCREF(col_name);
    // Py_INCREF(col_pylist_or_npy_arr);
    string col_name_str = PyObject_to_string(col_name);
    // Py_DECREF(col_name);
    Symbol col_head(move(col_name_str));

    PythonExpressionSystem::ExpressionSpanArguments col_list_spans;
    if (PyList_Check(col_pylist_or_npy_arr) == 1) {
      col_list_spans = py_list_to_spans(col_pylist_or_npy_arr);
    } else {
      col_list_spans = numpy_arr_to_spans_spans(reinterpret_cast<PyArrayObject *>(col_pylist_or_npy_arr));
    }

    // Py_DECREF(col_pylist_or_npy_arr);
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
  // Py_DECREF(table_dict);

  return PythonExpressionSystem::ComplexExpression("Table"_, {}, move(res_dynamics), {});
}

// steals reference to matrix_dict
PythonExpressionSystem::ComplexExpression Engine::pymatrix_to_table(PyObject *matrix_dict) {
  auto matrix = PyDict_GetItemString(matrix_dict, "data");
  // Py_INCREF(matrix);
  auto npy_matrix = reinterpret_cast<PyArrayObject *>(matrix);
  auto col_names = PyDict_GetItemString(matrix_dict, "col_names");
  // Py_INCREF(col_names);
  // Py_DECREF(matrix_dict);
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

  // Py_DECREF(col_names);
  // Py_DECREF(matrix);
  return get<PythonExpressionSystem::ComplexExpression>(move(result));
}

// steals reference to wrapper_dict
PythonExpressionSystem::Expression
Engine::pywrapper_to_table(PyObject *wrapper_dict) {
  auto table_dict = PyDict_GetItemString(wrapper_dict, "table");
  // Py_INCREF(table_dict);
  auto matrix_dict = PyDict_GetItemString(wrapper_dict, "matrix");
  // Py_INCREF(matrix_dict);

  PythonExpressionSystem::Expression table;
  if (table_dict != Py_None) {
    // todo
    table = pydict_col_or_spans_to_table_spans(table_dict);
  } else {
    table = pymatrix_to_table(matrix_dict);
  }
  // Py_DECREF(wrapper_dict);

  return table;
}

template <typename T>
PyObject *Engine::primitive_to_pyobject(T &&arg) {
  if constexpr (is_same_v<T, int32_t>) {
    return PyLong_FromLong(static_cast<int64_t>(arg));
  } else if constexpr (is_same_v<T, int64_t>) {
    return PyLong_FromLong(arg);
  } else if constexpr (is_same_v<T, double_t>) {
    return PyFloat_FromDouble(arg);
  } else if constexpr (is_same_v<T, string>) {
    const char *arg_c = arg.c_str();
    return PyUnicode_FromString(arg_c);
  } else {
    throw runtime_error("unsupported type: " + string(typeid(T).name()));
  }
}

// returns new reference
PyObject *Engine::single_span_list_to_pylist(PythonExpressionSystem::ComplexExpression &&list) {
  auto [list_unused_0, list_unused_1, list_unused_2, list_spans] =
    forward<decltype(list)>(list).decompose();
  auto list_it = make_move_iterator(list_spans.begin());
  auto span_arg = static_cast<PythonExpressionSystem::ExpressionSpanArgument>(*list_it);
  return visit(
    [this]<typename T>(Span<T> &&typed_span) -> PyObject * {
      auto size = typed_span.size();
      PyObject *py_list = PyList_New(size);
      for (size_t i = 0; i < size; i++) {
        PyObject *pyobject = primitive_to_pyobject<T>(move(typed_span[i]));
        PyList_SET_ITEM(py_list, i, pyobject);
      }
      return py_list;
    },
    forward<decltype(span_arg)>(span_arg)
  );
}

boss::expressions::ExpressionSpanArgument Engine::toBOSSExpression(PythonExpressionSystem::ExpressionSpanArgument&& span) {
  return std::visit(
      []<typename T>(boss::Span<T>&& typedSpan) -> boss::expressions::ExpressionSpanArgument {
        if constexpr (!(is_same_v<T, PyObject *> || is_same_v<T, PyObject *const>)) {
          return static_cast<boss::expressions::ExpressionSpanArgument>(std::move(typedSpan));
        } else {
          throw runtime_error("should not happen");
        }
      },
      std::move(span));
}

boss::Expression Engine::toBOSSExpression(PythonExpressionSystem::Expression&& expr) {
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
                [this](auto&& span) { return toBOSSExpression(std::forward<decltype(span)>(span)); });
            return boss::ComplexExpression(std::move(head), {}, std::move(bossDynamics),
                                            std::move(bossSpans));
          },
          [&](PyObject *&&e) -> boss::Expression {
            auto table = pydict_column_to_table_column(move(e));
            return toBOSSExpression(move(table));
          },
          [](auto&& otherTypes) -> boss::Expression { return otherTypes; }),
      std::move(expr));
}

PyObject *Engine::python_expression_to_pyobject(PythonExpressionSystem::Expression &&expr) {
  return std::visit(
    boss::utilities::overload(
        [&](PythonExpressionSystem::ComplexExpression&& e) -> PyObject * {
          return table_to_pydict_column(move(e));
        },
        [&](PyObject *&&e) -> PyObject * {
          return move(e);
        },
        [](__attribute__((unused)) auto&& otherTypes) -> PyObject * { 
          throw runtime_error("should not happen");
        }),
    std::move(expr));
}

#pragma endregion conversion_new

PythonExpressionSystem::Expression Engine::evaluate(PythonExpressionSystem::Expression &&e) {
  return visit(
      boss::utilities::overload(
          [this](PythonExpressionSystem::ComplexExpression &&expression) -> PythonExpressionSystem::Expression {

            // cout << expression.getHead().getName() << endl;

            // if (expression.getHead() == "Table"_) {
            //   cout << expression << endl;
            // }

            // cout << expression << endl;
            // cout << endl;

            // top-level
            auto [top_head, top_statics, top_dynamics, top_spans] =
                forward<decltype(expression)>(expression).decompose();

            // if (top_head == "to_boss"_) {
            //   // head = to_boss
            //   auto top_it = make_move_iterator(top_dynamics.begin());
            //   auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);
            //   auto result = python_expression_to_pyobject(evaluate(move(expr)));
            //   return pydict_column_to_table_column(result);
            // }

            // if (top_head == "to_python"_) {
            //   // head = to_python
            //   auto top_it = make_move_iterator(top_dynamics.begin());
            //   auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);
            //   auto result = get<PythonExpressionSystem::ComplexExpression>(evaluate(move(expr)));
            //   return table_to_pydict_column(move(result));
            //   // return PythonExpressionSystem::ComplexExpression("python"_, {}, {}, {});
            // }

            // if (top_head == "table_spans_to_matrix"_) {
            //   auto top_it = make_move_iterator(top_dynamics.begin());
            //   auto table = get<PythonExpressionSystem::ComplexExpression>(*top_it);

            //   PyObject* py_operator = PyObject_GetAttrString(rel_alg, "materialise_spans_into_matrix");
            //   if (py_operator == NULL) {
            //     PyErr_Print();
            //     throw runtime_error("error");
            //   }

            //   PyObject* result;
            //   if (PyCallable_Check(py_operator)) {
            //     // borrows references to args
            //     // returns new reference
            //     result = PyObject_CallFunction(py_operator, "O", table);
            //     if (result == NULL) {
            //       PyErr_Print();
            //       throw runtime_error("error");
            //     }
            //   } else {
            //     PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
            //     PyErr_Print();
            //     throw runtime_error("py_operator is not a callable object");
            //   }
            //   return result;
            // }

            if (top_head == "split_into_spans"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);

              auto evaluated_expr = evaluate(move(expr));
              // cout << evaluated_expr << endl;
              PyObject *table_pydict = python_expression_to_pyobject(move(evaluated_expr));

              int span_size = (1 << 20) / 4;

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "split_into_spans");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(py_operator, "Oi", table_pydict, span_size);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              return result;
            }

            if (top_head == "materialise_into_columns"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);

              auto evaluated_expr = evaluate(move(expr));
              // cout << evaluated_expr << endl;
              PyObject *table_pydict = python_expression_to_pyobject(move(evaluated_expr));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "materialise_into_columns");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(py_operator, "O", table_pydict);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              return result;
            }

            if (top_head == "DictionaryEncodedList"_) {
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto list = get<PythonExpressionSystem::ComplexExpression>(*top_it);
              return evaluate(move(list));
            }

            // def project(table, col_names)
            if (top_head == "project"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);
              PyObject *arg1 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1)));
              PyObject *arg2 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 2)));
              PyObject *arg3 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 3)));
              PyObject *arg4 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 4)));
              PyObject *arg5 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 5)));
              PyObject *arg6 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 6)));
              PyObject *arg7 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 7)));
              PyObject *arg8 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 8)));
              PyObject *arg9 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 9)));

              auto evaluated_expr = evaluate(move(expr));
              // cout << evaluated_expr << endl;
              PyObject *table_pydict = python_expression_to_pyobject(move(evaluated_expr));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "project");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(py_operator, "OOOOOOOOOO", table_pydict, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }
              return result;
            }

            // def select(table, key_col_names, boolean_ops, vals)
            if (top_head == "select"_) {
              // head = select
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);
              PyObject *arg1 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1)));
              PyObject *arg2 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 2)));
              PyObject *arg3 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 3)));

              PyObject *table_pydict = python_expression_to_pyobject(evaluate(move(expr)));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "select");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOO", table_pydict, arg1, arg2, arg3);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }

              return result;
            }

            // def equi_join(table_1, table_2, key_col_names_1, key_col_names_2)
            if (top_head == "equi_join"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr_1 = get<PythonExpressionSystem::ComplexExpression>(*top_it);
              auto expr_2 = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1));
              PyObject *arg1 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 2)));
              PyObject *arg2 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 3)));

              PyObject *table_pydict_1 = python_expression_to_pyobject(evaluate(move(expr_1)));
              PyObject *table_pydict_2 = python_expression_to_pyobject(evaluate(move(expr_2)));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "equi_join");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOO", table_pydict_1, table_pydict_2, arg1, arg2);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }

              return result;
            }

            // def aggregate(table, key_col_names, reduction_func, reduction_col_name)
            if (top_head == "aggregate"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              auto expr = get<PythonExpressionSystem::ComplexExpression>(*top_it);
              PyObject *arg1 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1)));
              PyObject *arg2 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 2)));
              PyObject *arg3 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 3)));
              PyObject *arg4 = single_span_list_to_pylist(get<PythonExpressionSystem::ComplexExpression>(*(top_it + 4)));

              PyObject *table_pydict = python_expression_to_pyobject(evaluate(move(expr)));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "aggregate");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOOO",table_pydict, arg1, arg2, arg3, arg4);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }

              return result;
            }

            // def aggregate_matrix(cnp.int_t[:, :] matrix, list[int] key_col_ixs_in, str reduction_func, int reduction_col_ix)
            if (top_head == "aggregate_matrix"_) {
              // head = project
              auto top_it = make_move_iterator(top_dynamics.begin());
              PyObject *matrix = get<PyObject *>(*top_it);
              auto key_col_ixs_expr = get<PythonExpressionSystem::ComplexExpression>(*(top_it + 1));
              auto reduction_func_expr = get<Symbol>(*(top_it + 2));
              auto reduction_col_ix = get<int>(*(top_it + 3));

              PyObject *key_col_ixs = single_span_list_to_pylist(move(key_col_ixs_expr));
              string reduction_func_str = reduction_func_expr.getName();
              PyObject *reduction_func = primitive_to_pyobject<string>(move(reduction_func_str));

              PyObject* py_operator = PyObject_GetAttrString(rel_alg, "aggregate");
              if (py_operator == NULL) {
                PyErr_Print();
                throw runtime_error("error");
              }

              PyObject* result;
              if (PyCallable_Check(py_operator)) {
                // borrows references to args
                // returns new reference
                result = PyObject_CallFunction(
                  py_operator, "OOOi", matrix, key_col_ixs, reduction_func, reduction_col_ix);
                if (result == NULL) {
                  PyErr_Print();
                  throw runtime_error("error");
                }
              } else {
                PyErr_SetString(PyExc_TypeError, "py_operator is not a callable object");
                PyErr_Print();
                throw runtime_error("py_operator is not a callable object");
              }

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
                // Py_DECREF(top_result);
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
                  // Py_DECREF(wrapper_dict);
                }
              }

              PyObject *top_result = PyRun_String(top_script, Py_file_input,
                                                  global_dict, local_dict);
              if (PyErr_Occurred()) {
                PyErr_Print();
                throw runtime_error("error in provided python code");
              }
              if (top_result != nullptr) {
                // Py_DECREF(top_result);
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
              // Py_INCREF(wrapper_dict);
              return pywrapper_to_table(wrapper_dict);
            }

            transform(make_move_iterator(top_dynamics.begin()),
                      make_move_iterator(top_dynamics.end()),
                      top_dynamics.begin(), [this](auto &&arg) {
                        auto result = evaluate(forward<decltype(arg)>(arg));
                        return get<PythonExpressionSystem::ComplexExpression>(move(result));
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
          [](auto &&arg) -> PythonExpressionSystem::Expression { 
            return forward<decltype(arg)>(arg); 
          }),
      forward<decltype(e)>(e));
};

boss::expressions::Expression Engine::evaluate_c(boss::expressions::Expression &&e) {
  auto result = evaluate(move(e));
  // cout << "ack" << endl;ś
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
  // gstate = PyGILState_Ensure();

  global_dict = PyDict_New();
  local_dict = PyDict_New();
  PyRun_String(R"(
import numpy as np
import sys
sys.path.append("/mnt/ubuntu-image-repos/BOSSNumpyEngine/rel_alg_cython_untyped")
#if 'rel_alg_cython_untyped' in sys.modules:
#  del sys.modules['rel_alg_cython_untyped']
#rel_alg_cython_untyped = importlib.import_module('rel_alg_cython_untyped')
#importlib.reload(rel_alg_cython_untyped)
  )", Py_file_input, global_dict, local_dict);

  main_module = PyImport_AddModule("__main__");
  rel_alg = PyImport_ImportModule("rel_alg_cython_untyped");
  rel_alg = PyImport_ReloadModule(rel_alg);
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

  // Py_DECREF(local_dict);
  local_dict = PyDict_New();
}

Engine::Engine(ull span_size_bytes) : span_size_bytes(span_size_bytes) {
  init_python_and_numpy();
  PyDict_SetItemString(global_dict, "__builtins__", PyEval_GetBuiltins());
}

Engine::~Engine() {
  // Py_DECREF(local_dict);
  // PyGILState_Release(gstate);
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
