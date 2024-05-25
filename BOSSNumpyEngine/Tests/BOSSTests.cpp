#define CATCH_CONFIG_RUNNER

#include "../Source/BOSSNumpyEngine.hpp"
#include <BOSS.hpp>
#include <ExpressionUtilities.hpp>

#include <catch2/catch.hpp>
#include <numeric>
#include <variant>

#define USE_NEW_TABLE_FORMAT
#define VERBOSE_OUTPUT

using boss::Expression;
using std::string;
using std::literals::string_literals::operator""s;
using boss::utilities::operator""_;
using Catch::Generators::random;
using Catch::Generators::take;
using Catch::Generators::values;
using std::vector;
using namespace Catch::Matchers;
using boss::expressions::CloneReason;
using boss::expressions::generic::get;
using boss::expressions::generic::get_if;
using boss::expressions::generic::holds_alternative;
namespace boss {
using boss::expressions::atoms::Span;
};

using intType = std::int32_t;

static std::vector<string>
    librariesToTest{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto createSpansInt = [](auto... values) {
  using SpanArguments = boss::expressions::ExpressionSpanArguments;
  std::vector<intType> v1 = {values...};
  auto s1 = boss::Span<intType>(std::move(v1));
  SpanArguments args;
  args.emplace_back(std::move(s1));
  return boss::expressions::ComplexExpression("List"_, {}, {}, std::move(args));
};

auto createSpansFloat = [](auto... values) {
  using SpanArguments = boss::expressions::ExpressionSpanArguments;
  std::vector<double_t> v1 = {values...};
  auto s1 = boss::Span<double_t>(std::move(v1));
  SpanArguments args;
  args.emplace_back(std::move(s1));
  return boss::expressions::ComplexExpression("List"_, {}, {}, std::move(args));
};

TEST_CASE("PROJECT", "[basics]") { // NOLINT
  boss::engines::numpy::Engine engine;
  auto eval = [&engine](boss::Expression &&expression) mutable {
    return engine.evaluate(std::move(expression));
  };

#ifdef USE_NEW_TABLE_FORMAT
  auto lineitem =
      "Table"_("l_orderkey"_(createSpansInt(1, 1, 2, 3)),
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
#else
  auto lineitem = "Table"_(
      "Column"_("l_orderkey"_, createSpansInt(1, 2, 3, 4)),
      "Column"_("l_partkey"_, createSpansInt(1, 2, 3, 4)),
      "Column"_("l_suppkey"_, createSpansInt(1, 2, 3, 4)),
      "Column"_("l_quantity"_, createSpansInt(17, 21, 8, 5)),
      "Column"_("l_extendedprice"_,
                createSpansFloat(17954.55, 34850.16, 7712.48, 25284.00)),
      "Column"_("l_discount"_, createSpansFloat(0.10, 0.05, 0.06, 0.06)),
      "Column"_("l_tax"_, createSpansFloat(0.02, 0.06, 0.02, 0.06)),
      "Column"_("l_returnflag"_, createSpansInt('N', 'N', 'A', 'A')),
      "Column"_("l_linestatus"_, createSpansInt('O', 'O', 'F', 'F')),
      "Column"_("l_shipdate"_, createSpansInt(1992, 1994, 1996, 1994)));
#endif

  auto lineitem_copy =
      "Table"_("l_orderkey"_(createSpansInt(1, 1, 2, 3)),
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

  SECTION("identity transformation") {
    auto const &result = eval(std::move(lineitem));

    CHECK(result == lineitem_copy); // NOLINT
  }
}

int main(int argc, char *argv[]) {
  Catch::Session session;
#ifdef VERBOSE_OUTPUT
  session.configData().showSuccessfulTests = true;
#endif
  session.cli(session.cli() |
              Catch::clara::Opt(librariesToTest, "library")["--library"]);
  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0) {
    return returnCode;
  }
  return session.run();
}