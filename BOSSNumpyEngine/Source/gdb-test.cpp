#include "BOSSNumpyEngine.hpp"

using namespace std;

using intType = int32_t;
using boss::engines::numpy::Engine;
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

auto createSpansInt = [](auto... values) {
  using SpanArguments = ExpressionSpanArguments;
  vector<intType> v1 = {values...};
  auto s1 = Span<intType>(move(v1));
  SpanArguments args;
  args.emplace_back(move(s1));
  return ComplexExpression("List"_, {}, {}, move(args));
};

auto createSpansFloat = [](auto... values) {
  using SpanArguments = ExpressionSpanArguments;
  vector<double_t> v1 = {values...};
  auto s1 = Span<double_t>(move(v1));
  SpanArguments args;
  args.emplace_back(move(s1));
  return ComplexExpression("List"_, {}, {}, move(args));
};

auto create_lineitem() {
  return "Table"_("l_orderkey"_(createSpansInt(1, 1, 2, 3)),
                  "l_partkey"_(createSpansInt(1, 2, 3, 4)),
                  "l_suppkey"_(createSpansInt(1, 2, 3, 4)),
                  "l_returnflag"_(createSpansInt('N', 'N', 'A', 'A')),
                  "l_linestatus"_(createSpansInt('O', 'O', 'F', 'F')),
                  "l_quantity"_(createSpansInt(17, 21, 8, 5)),
                  "l_extendedprice"_(
                      createSpansFloat(17954.55, 34850.16, 7712.48, 25284.00)),
                  "l_discount"_(createSpansFloat(0.10, 0.05, 0.06, 0.06)),
                  "l_tax"_(createSpansFloat(0.02, 0.06, 0.02, 0.06)),
                  "l_shipdate"_(createSpansInt(1992, 1994, 1996, 1994)));
}

int main(int argc, char *argv[]) {
  Engine engine;

  auto lineitem = create_lineitem();

  auto const &result = engine.evaluate(move("Python"_(
      "print(foo)\nprint('hello')"_, "where"_("foo"_, move(lineitem)))));

  return 0;
}
