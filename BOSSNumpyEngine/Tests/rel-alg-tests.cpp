#include "../Source/BOSSNumpyEngine.hpp"

// template<typename T>
// auto create_span(auto... values) {
//   vector<T> v1 = {values...};
//   auto s1 = Span<T>(move(v1));
//   boss::expressions::ExpressionSpanArguments args;
//   args.emplace_back(move(s1));
//   return ComplexExpression("List"_, {}, {}, move(args));
// };

// auto createSpansInt(auto... values) {
//   return create_span<int32_t>(values...);
// }

// auto createSpansDouble(auto... values) {
//   return create_span<double>(values...);
// }

// auto createSpansStr(auto... values) {
//   return create_span<string>(values...);
// }

// auto createSpansInt = [](auto... values) {
//   // using SpanArguments = ExpressionSpanArguments;
//   vector<intType> v1 = {values...};
//   auto s1 = Span<intType>(move(v1));
//   boss::expressions::ExpressionSpanArguments args;
//   args.emplace_back(move(s1));
//   return ComplexExpression("List"_, {}, {}, move(args));
// };

// auto createSpansDouble = [](auto... values) {
//   // using SpanArguments = ExpressionSpanArguments;
//   vector<double> v1 = {values...};
//   auto s1 = Span<double>(move(v1));
//   boss::expressions::ExpressionSpanArguments args;
//   args.emplace_back(move(s1));
//   return ComplexExpression("List"_, {}, {}, move(args));
// };

// auto createSpansStr = [](auto... values) {
//   // using SpanArguments = ExpressionSpanArguments;
//   vector<string> v1 = {values...};
//   auto s1 = Span<string>(move(v1));
//   boss::expressions::ExpressionSpanArguments args;
//   args.emplace_back(move(s1));
//   return ComplexExpression("List"_, {}, {}, move(args));
// };

// pydict_column = {"col1": npy_arr1, "col2": npy_arr2}

int main() {
  boss::engines::numpy::Engine engine(ENGINE_SPAN_SIZE_BYTES);

  // auto expectedCol1 = boss::Span<int32_t>{vector<int32_t>{1, 2, 4, 3}};
  // auto expectedCol2 = boss::Span<double>{vector<double>{3.14, 4.2, 5.1, 0.0005}};
  // auto expectedCol3 = boss::Span<string>{vector<string>{"one", "two", "three", "four"}};
  // auto expectedTable = "Table"_("col1"_("List"_(move(expectedCol1))),
  //                               "col2"_("List"_(move(expectedCol2))),
  //                               "col3"_("List"_(move(expectedCol3))));

  

  auto table_1 = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{1, 2, 3}})),
    "col2"_("List"_(boss::Span<double>{vector<double>{0.8, 3.14, 2.42}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1}}))
  );
  auto project_query = "project"_(
    move(table_1),
    "List"_(boss::Span<string>{vector<string>{"col2", "col3"}})
  );
  auto const project_res = engine.evaluate(move(project_query));
  cout << "project_res " << endl << project_res << endl;

  // auto table_1 = "Table"_(
  //   "col1"_(createSpansInt(1, 2, 3)),
  //   "col2"_(createSpansDouble(0.8, 3.14, 2.42)),
  //   "col3"_(createSpansInt(0, 0, 1)),
  // );
  // auto project_query = "project"_(
  //   move(table_1),
  //   createSpansStr("col2", "col3"),
  // );
  // auto const project_res = engine.evaluate(move(project_query));
  // auto project_expected = "Table"_(
  //   "col2"_(createSpansDouble(0.8, 3.14, 2.42)),
  //   "col3"_(createSpansInt(0, 0, 1)),
  // );
  // cout << "project_res " << endl << project_res << endl;
  // cout << "project_expected " << endl << project_expected << endl;
  // cout << endl;

  return 0;
}
