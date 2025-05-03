#include "../Source/BOSSNumpyEngine.hpp"

using ExpressionBuilder = boss::utilities::ExpressionBuilder;
static ExpressionBuilder operator""_(const char* name, size_t /*unused*/) {
  return ExpressionBuilder(name);
};

// Base case for variadic template recursion
template <typename U>
void addToVector(__attribute__((unused)) std::vector<U>& vec) {}

// Recursive variadic template function
template <typename T, typename... Args>
void addToVector(std::vector<T>& vec, const T& first, const Args&... args) {
    vec.push_back(first);
    addToVector<T>(vec, args...);
}

// Function that takes a variable number of arguments
template <typename T, typename... Args>
std::vector<T> makeVector(const Args&... args) {
    std::vector<T> vec;
    addToVector<T>(vec, args...);
    return vec;
}

// Function that takes a variable number of arguments
template <typename T, typename... Args>
boss::Span<T> makeSpan(const Args&... args) {
    return boss::Span<T>{makeVector<T>(args...)};
}

// Function that takes a variable number of arguments
template <typename T, typename... Args>
boss::ComplexExpression makeList(const Args&... args) {
    return "List"_(makeSpan<T>(args...));
}

template <typename... Args>
boss::ComplexExpression int_list(const Args&... args) {
    return makeList<int32_t>(args...);
}

template <typename... Args>
boss::ComplexExpression double_list(const Args&... args) {
    return makeList<double>(args...);
}

template <typename... Args>
boss::ComplexExpression string_list(const Args&... args) {
    return makeList<std::string>(args...);
}

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

  auto table_1 = "Table"_(
    "col1"_(int_list(1, 2, 3)),
    "col2"_(double_list(0.8, 3.14, 2.42)),
    "col3"_(int_list(0, 0, 1))
  );
  auto project_query = "project"_(
    shallow_copy(table_1),
    string_list("col2", "col3")
  );
  auto const project_res = engine.evaluate_c(shallow_copy(project_query));
  auto project_expected = "Table"_(
    "col2"_(double_list(0.8, 3.14, 2.42)),
    "col3"_(int_list(0, 0, 1))
  );
  cout << "project_res " << endl << project_res << endl;
  cout << "project_expected " << endl << project_expected << endl;
  cout << endl;
  assert(project_res == project_expected);

  auto select_query = "select"_(
    shallow_copy(table_1),
    string_list("col3", "col1"),
    string_list("==", "<"),
    double_list(0.0, 1.5)
  );
  auto const select_res = engine.evaluate_c(shallow_copy(select_query));
  auto select_expected = "Table"_(
    "col1"_(int_list(1)),
    "col2"_(double_list(0.8)),
    "col3"_(int_list(0))
  );
  cout << "select_res " << endl << select_res << endl;
  cout << "select_expected " << endl << select_expected << endl;
  cout << endl;
  assert(select_res == select_expected);

  auto table_2 = "Table"_(
    "col1"_(int_list(10, 5, 1)),
    "col2"_(int_list(3, 3, 4)),
    "col3"_(int_list(6, 7, 8))
  );
  auto table_3 = "Table"_(
    "col1"_(int_list(10, 5, 2)),
    "col2"_(int_list(5, 3, 4)),
    "col4"_(int_list(9, 2, 1))
  );
  auto join_query = "equi_join"_(
    shallow_copy(table_2),
    shallow_copy(table_3),
    string_list("col1", "col2"),
    string_list("col1", "col2")
  );
  auto const join_res = engine.evaluate_c(shallow_copy(join_query));
  auto join_expected = "Table"_(
    "col1"_(int_list(5)),
    "col2"_(int_list(3)),
    "col3"_(int_list(7)),
    "col4"_(int_list(2))
  );
  cout << "join_res " << endl << join_res << endl;
  cout << "join_expected " << endl << join_expected << endl;
  cout << endl;
  assert(join_res == join_expected);

  auto table_4 = "Table"_(
    "col1"_(int_list(0, 1, 0, 1, 0, 1, 0, 1)),
    "col2"_(int_list(0, 0, 1, 1, 0, 0, 1, 1)),
    "col3"_(int_list(1, 2, 3, 4, 1, 2, 3, 4))
  );
  auto aggregate_query = "aggregate"_(
    shallow_copy(table_4),
    string_list("col1", "col2"),
    "sum"_,
    "col3"_
  );
  auto const aggregate_res = engine.evaluate_c(shallow_copy(aggregate_query));
  auto aggregate_expected = "Table"_(
    "col1"_(int_list(0, 1, 0, 1)),
    "col2"_(int_list(0, 0, 1, 1)),
    "col3"_(int_list(2, 4, 6, 8))
  );
  cout << "aggregate_res " << endl << aggregate_res << endl;
  cout << "aggregate_expected " << endl << aggregate_expected << endl;
  cout << endl;
  assert(aggregate_res == aggregate_expected);

  return 0;
}
