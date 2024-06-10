#include "../Source/BOSSNumpyEngine.hpp"

using ExpressionBuilder = boss::utilities::ExpressionBuilder;
static ExpressionBuilder operator""_(const char* name, size_t /*unused*/) {
  return ExpressionBuilder(name);
};

// pydict_column = {"col1": npy_arr1, "col2": npy_arr2}

int main() {
  boss::engines::numpy::Engine engine(ENGINE_SPAN_SIZE_BYTES);

  auto table_1 = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{1, 2, 3}})),
    "col2"_("List"_(boss::Span<double>{vector<double>{0.8, 3.14, 2.42}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1}}))
  );
  auto project_query = "project"_(
    move(table_1),
    "List"_(boss::Span<string>{vector<string>{"col2", "col3"}})
  );
  auto const project_res = engine.evaluate_c(move(project_query));
  cout << "project_res " << endl << project_res << endl;
  auto project_expected = "Table"_(
    "col2"_("List"_(boss::Span<double>{vector<double>{0.8, 3.14, 2.42}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1}}))
  );
  assert(project_res == project_expected);



  return 0;
}
