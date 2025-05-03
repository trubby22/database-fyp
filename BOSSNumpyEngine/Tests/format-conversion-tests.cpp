#include "../Source/BOSSNumpyEngine.hpp"

using ExpressionBuilder = boss::utilities::ExtensibleExpressionBuilder<>;
static ExpressionBuilder operator""_(const char* name, size_t /*unused*/) {
  return ExpressionBuilder(name);
};

static boss::ComplexExpression shallow_copy(boss::ComplexExpression const& e) {
  auto const& head = e.getHead();
  auto const& dynamics = e.getDynamicArguments();
  auto const& spans = e.getSpanArguments();
  boss::ExpressionArguments dynamicsCopy;
  std::transform(dynamics.begin(), dynamics.end(), std::back_inserter(dynamicsCopy),
                 [](auto const& arg) {
                   return std::visit(
                       boss::utilities::overload(
                           [&](boss::ComplexExpression const& expr) -> boss::Expression {
                             return shallow_copy(expr);
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
            // TODO: this would still keep const spans for bools, need to fix later
            return SpanType(typedSpan.begin(), typedSpan.size(), []() {});
          } else {
            // force non-const value for now (otherwise expressions cannot be moved)
            auto* ptr = const_cast<T*>(typedSpan.begin()); // NOLINT
            return boss::Span<T>(ptr, typedSpan.size(), []() {});
          }
        },
        span);
  });
  return boss::ComplexExpression(head, {}, std::move(dynamicsCopy), std::move(spansCopy));
}

// pydict_column = {"col1": npy_arr1, "col2": npy_arr2}

int main() {
  boss::engines::numpy::Engine engine(ENGINE_SPAN_SIZE_BYTES);

  auto table_spans = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{1, 2}}, boss::Span<int32_t>{vector<int32_t>{3, 4}})),
    "col2"_("List"_(boss::Span<int32_t>{vector<int32_t>{5, 6}}, boss::Span<int32_t>{vector<int32_t>{7, 8}}))
  );

  auto set_up_spans = "Python"_(R"(

table = {
    'col1': [
        np.array([1, 2], dtype=np.int32),
        np.array([3, 4], dtype=np.int32),
    ], 
    'col2': [
        np.array([5, 6], dtype=np.int32),
        np.array([7, 8], dtype=np.int32),
    ],
}
wrapper = {
    'table': table,
    'matrix': None
}

print(table)

)"_, 
    "Where"_());
  auto extract_spans = "get_python_var"_("wrapper"_);
  auto const set_up_spans_res = engine.evaluate_c(shallow_copy(set_up_spans));
  auto const extract_spans_res = engine.evaluate_c(shallow_copy(extract_spans));
  cout << "table_spans " << table_spans << endl;
  cout << "extract_spans_res " << extract_spans_res << endl;
  cout << endl;
  assert(extract_spans_res == table_spans);

// ==========================================================================================

  auto set_up_columns = "Python"_(R"(

table = {
    'col1': [
        np.array([1, 2, 3, 4], dtype=np.int32),
    ], 
    'col2': [
        np.array([5, 6, 7, 8], dtype=np.int32),
    ],
}
wrapper = {
    'table': table,
    'matrix': None
}

print(table)

)"_, 
    "Where"_());
  auto extract_columns = "get_python_var"_("wrapper"_);
  auto const set_up_columns_res = engine.evaluate_c(shallow_copy(set_up_columns));
  auto const extract_columns_res = engine.evaluate_c(shallow_copy(extract_columns));
  cout << "table_spans " << table_spans << endl;
  cout << "extract_columns_res " << extract_columns_res << endl;
  cout << endl;
  assert(extract_columns_res == table_spans);

// ==========================================================================================

  auto set_up_matrix = "Python"_(R"(

matrix = np.array([
    [1, 2, 3, 4],
    [5, 6, 7, 8],
], dtype=np.int32)
wrapper = {
    'table': None,
    'matrix': matrix
}

print(matrix)

)"_, 
    "Where"_());
  auto extract_matrix = "get_python_var"_("wrapper"_);
  auto const set_up_matrix_res = engine.evaluate_c(shallow_copy(set_up_matrix));
  auto const extract_matrix_res = engine.evaluate_c(shallow_copy(extract_matrix));
  cout << "table_spans " << table_spans << endl;
  cout << "extract_matrix_res " << extract_matrix_res << endl;
  cout << endl;
  assert(extract_matrix_res == table_spans);

  return 0;
}
