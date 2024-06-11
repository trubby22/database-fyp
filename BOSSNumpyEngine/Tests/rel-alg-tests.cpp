#include "../Source/BOSSNumpyEngine.hpp"

using ExpressionBuilder = boss::utilities::ExpressionBuilder;
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

  auto table_1 = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{1, 2, 3}})),
    "col2"_("List"_(boss::Span<double>{vector<double>{0.8, 3.14, 2.42}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1}}))
  );
  auto project_query = "project"_(
    shallow_copy(table_1),
    "List"_(boss::Span<string>{vector<string>{"col2", "col3"}})
  );
  auto const project_res = engine.evaluate_c(move(project_query));
  auto project_expected = "Table"_(
    "col2"_("List"_(boss::Span<double>{vector<double>{0.8, 3.14, 2.42}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1}}))
  );
  cout << "project_res " << endl << project_res << endl;
  cout << "project_expected " << endl << project_expected << endl;
  cout << endl;
  assert(project_res == project_expected);

  auto select_query = "select"_(
    shallow_copy(table_1),
    "List"_(boss::Span<string>{vector<string>{"col3", "col1"}}),
    "List"_(boss::Span<string>{vector<string>{"==", "<"}}),
    "List"_(boss::Span<double>{vector<double>{0.0, 1.5}})
  );
  auto const select_res = engine.evaluate_c(move(select_query));
  auto select_expected = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{1}})),
    "col2"_("List"_(boss::Span<double>{vector<double>{0.8}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{0}}))
  );
  cout << "select_res " << endl << select_res << endl;
  cout << "select_expected " << endl << select_expected << endl;
  cout << endl;
  assert(select_res == select_expected);

  auto table_2 = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{10, 5, 1}})),
    "col2"_("List"_(boss::Span<int32_t>{vector<int32_t>{3, 3, 4}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{6, 7, 8}}))
  );
  auto table_3 = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{10, 5, 2}})),
    "col2"_("List"_(boss::Span<int32_t>{vector<int32_t>{5, 3, 4}})),
    "col4"_("List"_(boss::Span<int32_t>{vector<int32_t>{9, 2, 1}}))
  );
  auto join_query = "equi_join"_(
    shallow_copy(table_2),
    shallow_copy(table_3),
    "List"_(boss::Span<string>{vector<string>{"col1", "col2"}}),
    "List"_(boss::Span<string>{vector<string>{"col1", "col2"}})
  );
  auto const join_res = engine.evaluate_c(move(join_query));
  auto join_expected = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{5}})),
    "col2"_("List"_(boss::Span<int32_t>{vector<int32_t>{3}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{7}})),
    "col4"_("List"_(boss::Span<int32_t>{vector<int32_t>{2}}))
  );
  cout << "join_res " << endl << join_res << endl;
  cout << "join_expected " << endl << join_expected << endl;
  cout << endl;
  assert(join_res == join_expected);

  auto table_4 = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 1, 0, 1, 0, 1, 0, 1}})),
    "col2"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1, 1, 0, 0, 1, 1}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{1, 2, 3, 4, 1, 2, 3, 4}}))
  );
  auto aggregate_query = "aggregate"_(
    shallow_copy(table_4),
    "List"_(boss::Span<string>{vector<string>{"col1", "col2"}}),
    "sum"_,
    "col3"_
  );
  auto const aggregate_res = engine.evaluate_c(move(aggregate_query));
  auto aggregate_expected = "Table"_(
    "col1"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 1, 0, 1}})),
    "col2"_("List"_(boss::Span<int32_t>{vector<int32_t>{0, 0, 1, 1}})),
    "col3"_("List"_(boss::Span<int32_t>{vector<int32_t>{2, 4, 6, 8}}))
  );
  cout << "aggregate_res " << endl << aggregate_res << endl;
  cout << "aggregate_expected " << endl << aggregate_expected << endl;
  cout << endl;
  assert(aggregate_res == aggregate_expected);

  auto table = "Table"_(
    "col1"_("List"_(
      boss::Span<int32_t>{vector<int32_t>{1, 2}}, 
      boss::Span<int32_t>{vector<int32_t>{3, 4}}
      )),
    "col2"_("List"_(
      boss::Span<int32_t>{vector<int32_t>{5, 6}}, 
      boss::Span<int32_t>{vector<int32_t>{7, 8}}
      ))
  );

  return 0;
}
