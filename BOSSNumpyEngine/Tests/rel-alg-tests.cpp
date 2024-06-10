#include "../Source/BOSSNumpyEngine.hpp"

auto create_int_col(int32_t... values) {
  return create_span<int32_t>(values...);
}

auto create_double_col(double... values) {
  return create_span<double>(values...);
}

auto create_str_col(string... values) {
  return create_span<string>(values...);
}

template<typename... T>
auto create_span(T... values) {
  vector<T> v1 = {values...};
  auto s1 = Span<T>(move(v1));
  boss::expressions::ExpressionSpanArguments args;
  args.emplace_back(move(s1));
  return ComplexExpression("List"_, {}, {}, move(args));
};

// pydict_column = {"col1": npy_arr1, "col2": npy_arr2}

int main() {
  boss::engines::numpy::Engine engine(ENGINE_SPAN_SIZE_BYTES);
  auto table_1 = "Table"_(
    "col1"_(create_int_col(1, 2, 3)),
    "col2"_(create_double_col(0.8, 3.14, 2.42)),
    "col3"_(create_int_col(0, 0, 1)),
  );
  auto project_query = "project"_(
    move(table_1),
    create_str_col("col2", "col3"),
  );
  auto const project_res = engine.evaluate(move(project_query));
  auto project_expected = "Table"_(
    "col2"_(create_double_col(0.8, 3.14, 2.42)),
    "col3"_(create_int_col(0, 0, 1)),
  );
  cout << "project_res " << endl << project_res << endl;
  cout << "project_expected " << endl << project_expected << endl;
  cout << endl;

  return 0;
}
