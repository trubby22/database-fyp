#define CATCH_CONFIG_RUNNER

#include "../Source/BOSSNumPyEngine.hpp"
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

TEST_CASE("TPCH", "[basics]") { // NOLINT
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

  // SECTION("q3-no-top") {
  //   auto const &result = eval("Order"_(
  //       "Group"_(
  //           "Project"_(
  //               "Join"_(
  //                   "Project"_("ORDERS"_,
  //                              "As"_("o_orderkey"_, "o_orderkey"_,
  //                                    "o_orderdate"_, "o_orderdate"_)),
  //                   "Project"_(
  //                       "Join"_(
  //                           "Project"_(
  //                               "Join"_(
  //                                   "Project"_(
  //                                       "Select"_(
  //                                           "Project"_("PART"_,
  //                                                      "As"_("p_partkey"_,
  //                                                            "p_partkey"_,
  //                                                            "p_retailprice"_,
  //                                                            "p_retailprice"_)),
  //                                           "Where"_("And"_(
  //                                               "Greater"_("p_retailprice"_,
  //                                                          1006.05), // NOLINT
  //                                               "Greater"_(1080.1,   // NOLINT
  //                                                          "p_retailprice"_)))),
  //                                       "As"_("p_partkey"_, "p_partkey"_,
  //                                             "p_retailprice"_,
  //                                             "p_retailprice"_)),
  //                                   "Project"_(
  //                                       "Join"_(
  //                                           "Project"_(
  //                                               "Join"_(
  //                                                   "Project"_(
  //                                                       "NATION"_,
  //                                                       "As"_("n_name"_,
  //                                                             "n_name"_,
  //                                                             "n_nationkey"_,
  //                                                             "n_nationkey"_)),
  //                                                   "Project"_(
  //                                                       "SUPPLIER"_,
  //                                                       "As"_("s_suppkey"_,
  //                                                             "s_suppkey"_,
  //                                                             "s_nationkey"_,
  //                                                             "s_nationkey"_)),
  //                                                   "Where"_("Equal"_(
  //                                                       "n_nationkey"_,
  //                                                       "s_nationkey"_))),
  //                                               "As"_("n_name"_, "n_name"_,
  //                                                     "s_suppkey"_,
  //                                                     "s_suppkey"_)),
  //                                           "Project"_("PARTSUPP"_,
  //                                                      "As"_("ps_partkey"_,
  //                                                            "ps_partkey"_,
  //                                                            "ps_suppkey"_,
  //                                                            "ps_suppkey"_,
  //                                                            "ps_supplycost"_,
  //                                                            "ps_supplycost"_)),
  //                                           "Where"_("Equal"_("s_suppkey"_,
  //                                                             "ps_suppkey"_))),
  //                                       "As"_("n_name"_, "n_name"_,
  //                                             "ps_partkey"_, "ps_partkey"_,
  //                                             "ps_suppkey"_, "ps_suppkey"_,
  //                                             "ps_supplycost"_,
  //                                             "ps_supplycost"_)),
  //                                   "Where"_(
  //                                       "Equal"_("p_partkey"_, "ps_partkey"_))),
  //                               "As"_("n_name"_, "n_name"_, "ps_partkey"_,
  //                                     "ps_partkey"_, "ps_suppkey"_,
  //                                     "ps_suppkey"_, "ps_supplycost"_,
  //                                     "ps_supplycost"_)),
  //                           "Project"_("LINEITEM"_,
  //                                      "As"_("l_partkey"_, "l_partkey"_,
  //                                            "l_suppkey"_, "l_suppkey"_,
  //                                            "l_orderkey"_, "l_orderkey"_,
  //                                            "l_extendedprice"_,
  //                                            "l_extendedprice"_, "l_discount"_,
  //                                            "l_discount"_, "l_quantity"_,
  //                                            "l_quantity"_)),
  //                           "Where"_(
  //                               "Equal"_("List"_("ps_partkey"_, "ps_suppkey"_),
  //                                        "List"_("l_partkey"_, "l_suppkey"_)))),
  //                       "As"_("n_name"_, "n_name"_, "ps_supplycost"_,
  //                             "ps_supplycost"_, "l_orderkey"_, "l_orderkey"_,
  //                             "l_extendedprice"_, "l_extendedprice"_,
  //                             "l_discount"_, "l_discount"_, "l_quantity"_,
  //                             "l_quantity"_)),
  //                   "Where"_("Equal"_("o_orderkey"_, "l_orderkey"_))),
  //               "As"_("nation"_, "n_name"_, "o_year"_, "Year"_("o_orderdate"_),
  //                     "amount"_,
  //                     "Minus"_("Times"_("l_extendedprice"_,
  //                                       "Minus"_(1.0, "l_discount"_)),
  //                              "Times"_("ps_supplycost"_, "l_quantity"_)))),
  //           "By"_("nation"_, "o_year"_), "As"_("amount"_, "Sum"_("amount"_))),
  //       "By"_("nation"_, "o_year"_, "desc"_)));

  //   CHECK(result ==
  //         "List"_("List"_(34850.16 * 0.05 + 25284.00 * 0.06))); // NOLINT
  // }

  // SECTION("q3-no-top") {
  //   auto const &result = eval("Group"_(
  //       "Project"_(
  //           "Join"_(
  //               "Project"_(
  //                   "Join"_("Project"_(
  //                               "Select"_(
  //                                   "Project"_("CUSTOMER"_,
  //                                              "As"_("c_custkey"_,
  //                                              "c_custkey"_,
  //                                                    "c_mktsegment"_,
  //                                                    "c_mktsegment"_)),
  //                                   "Where"_("StringContainsQ"_("c_mktsegment"_,
  //                                                               "BUILDING"))),
  //                               "As"_("c_custkey"_, "c_custkey"_,
  //                                     "c_mktsegment"_, "c_mktsegment"_)),
  //                           "Select"_(
  //                               "Project"_("ORDERS"_,
  //                                          "As"_("o_orderkey"_,
  //                                          "o_orderkey"_,
  //                                                "o_orderdate"_,
  //                                                "o_orderdate"_,
  //                                                "o_custkey"_, "o_custkey"_,
  //                                                "o_shippriority"_,
  //                                                "o_shippriority"_)),
  //                               "Where"_("Greater"_("DateObject"_("1995-03-15"),
  //                                                   "o_orderdate"_))),
  //                           "Where"_("Equal"_("c_custkey"_, "o_custkey"_))),
  //                   "As"_("o_orderkey"_, "o_orderkey"_, "o_orderdate"_,
  //                         "o_orderdate"_, "o_custkey"_, "o_custkey"_,
  //                         "o_shippriority"_, "o_shippriority"_)),
  //               "Project"_(
  //                   "Select"_(
  //                       "Project"_(
  //                           "LINEITEM"_,
  //                           "As"_("l_orderkey"_, "l_orderkey"_,
  //                           "l_discount"_,
  //                                 "l_discount"_, "l_shipdate"_,
  //                                 "l_shipdate"_, "l_extendedprice"_,
  //                                 "l_extendedprice"_)),
  //                       "Where"_("Greater"_("l_shipdate"_,
  //                                           "DateObject"_("1995-03-15")))),
  //                   "As"_("l_orderkey"_, "l_orderkey"_, "l_discount"_,
  //                         "l_discount"_, "l_extendedprice"_,
  //                         "l_extendedprice"_)),
  //               "Where"_("Equal"_("o_orderkey"_, "l_orderkey"_))),
  //           "As"_("expr1009"_,
  //                 "Times"_("l_extendedprice"_, "Minus"_(1.0, "l_discount"_)),
  //                 "l_extendedprice"_, "l_extendedprice"_, "l_orderkey"_,
  //                 "l_orderkey"_, "o_orderdate"_, "o_orderdate"_,
  //                 "o_shippriority"_, "o_shippriority"_)),
  //       "By"_("l_orderkey"_),
  //       "As"_("revenue"_, "Sum"_("expr1009"_), "o_orderdate"_,
  //             "Min"_("o_orderdate"_), "o_shippriority"_,
  //             "Min"_("o_shippriority"_))));

  //   CHECK(result ==
  //         "List"_("List"_(34850.16 * 0.05 + 25284.00 * 0.06))); // NOLINT
  // }

  // SECTION("q6") {
  //   auto const &result = eval("Group"_(
  //       "Project"_(
  //           "Select"_(
  //               "Project"_(std::move(lineitem),
  //                          "As"_("l_quantity"_, "l_quantity"_, "l_discount"_,
  //                                "l_discount"_, "l_shipdate"_, "l_shipdate"_,
  //                                "l_extendedprice"_, "l_extendedprice"_)),
  //               "Where"_("And"_("Greater"_(24, "l_quantity"_),
  //                               "Greater"_("l_discount"_, 0.0499),
  //                               "Greater"_(0.07001, "l_discount"_),
  //                               "Greater"_(1995, "l_shipdate"_),
  //                               "Greater"_("l_shipdate"_, 1993)))),
  //           "As"_("revenue"_, "Times"_("l_extendedprice"_, "l_discount"_))),
  //       "Sum"_("revenue"_)));

  //   CHECK(result ==
  //         "List"_("List"_(34850.16 * 0.05 + 25284.00 * 0.06))); // NOLINT
  // }

  // SECTION("q1") {
  //   auto const &result = eval("Order"_(
  //       "Project"_(
  //           "Group"_(
  //               "Project"_(
  //                   "Project"_(
  //                       "Project"_(
  //                           "Select"_(
  //                               "Project"_(
  //                                   "LINEITEM"_,
  //                                   "As"_("l_quantity"_, "l_quantity"_,
  //                                         "l_discount"_, "l_discount"_,
  //                                         "l_shipdate"_, "l_shipdate"_,
  //                                         "l_extendedprice"_,
  //                                         "l_extendedprice"_,
  //                                         "l_returnflag"_, "l_returnflag"_,
  //                                         "l_linestatus"_, "l_linestatus"_,
  //                                         "l_tax"_, "l_tax"_)),
  //                               "Where"_("Greater"_("DateObject"_("1998-08-31"),
  //                                                   "l_shipdate"_))),
  //                           "As"_("l_returnflag"_, "l_returnflag"_,
  //                                 "l_linestatus"_, "l_linestatus"_,
  //                                 "l_quantity"_, "l_quantity"_,
  //                                 "l_extendedprice"_, "l_extendedprice"_,
  //                                 "l_discount"_, "l_discount"_, "calc1"_,
  //                                 "Minus"_(1.0, "l_discount"_), "calc2"_,
  //                                 "Plus"_("l_tax"_, 1.0))),
  //                       "As"_("l_returnflag"_, "l_returnflag"_,
  //                       "l_linestatus"_,
  //                             "l_linestatus"_, "l_quantity"_, "l_quantity"_,
  //                             "l_extendedprice"_, "l_extendedprice"_,
  //                             "l_discount"_, "l_discount"_, "disc_price"_,
  //                             "Times"_("l_extendedprice"_, "calc1"_),
  //                             "calc2"_, "calc2"_)),
  //                   "As"_("l_returnflag"_, "l_returnflag"_, "l_linestatus"_,
  //                         "l_linestatus"_, "l_quantity"_, "l_quantity"_,
  //                         "l_extendedprice"_, "l_extendedprice"_,
  //                         "l_discount"_, "l_discount"_, "disc_price"_,
  //                         "disc_price"_, "calc"_, "Times"_("disc_price"_,
  //                         "calc2"_))),
  //               "By"_("l_returnflag"_, "l_linestatus"_),
  //               // disable one aggregate to limit it to 5 aggregate max for
  //               now
  //               // (to reduce compilation time for the hazard-adaptive
  //               engine) "As"_("sum_qty"_, "Sum"_("l_quantity"_),
  //               "sum_base_price"_,
  //                     "Sum"_("l_extendedprice"_), "sum_disc_price"_,
  //                     "Sum"_("disc_price"_), "sum_charges"_,
  //                     "Sum"_("calc"_), /*"sum_disc"_,
  //                     "Sum"_("l_discount"_),*/ "count_order"_,
  //                     "Count"_("l_quantity"_))),
  //           "As"_("l_returnflag"_, "l_returnflag"_, "l_linestatus"_,
  //                 "l_linestatus"_, "sum_qty"_, "sum_qty"_, "sum_base_price"_,
  //                 "sum_base_price"_, "sum_disc_price"_, "sum_disc_price"_,
  //                 "sum_charges"_, "sum_charges"_, "avg_qty"_,
  //                 "Divide"_("sum_qty"_, "count_order"_), "avg_price"_,
  //                 "Divide"_("sum_base_price"_, "count_order"_),
  //                 /*"avg_disc"_, "Divide"_("sum_disc"_, "count_order"_),*/
  //                 "count_order"_, "count_order"_)),
  //       "By"_("l_returnflag"_, "l_linestatus"_)));

  //   CHECK(result ==
  //         "List"_("List"_(34850.16 * 0.05 + 25284.00 * 0.06))); // NOLINT
  // }

  // SECTION("q3-first-join-only") {
  //   auto const &result = eval("Order"_(
  //       "Project"_(
  //           "Join"_("Project"_(
  //                       "Select"_(
  //                           "Project"_("CUSTOMER"_,
  //                                      "As"_("c_custkey"_, "c_custkey"_,
  //                                            "c_mktsegment"_,
  //                                            "c_mktsegment"_)),
  //                           "Where"_("StringContainsQ"_("c_mktsegment"_,
  //                                                       "BUILDING"))),
  //                       "As"_("c_custkey"_, "c_custkey"_, "c_mktsegment"_,
  //                             "c_mktsegment"_)),
  //                   "Select"_(
  //                       "Project"_("ORDERS"_,
  //                                  "As"_("o_orderkey"_, "o_orderkey"_,
  //                                        "o_orderdate"_, "o_orderdate"_,
  //                                        "o_custkey"_, "o_custkey"_,
  //                                        "o_shippriority"_,
  //                                        "o_shippriority"_)),
  //                       "Where"_("Greater"_("DateObject"_("1995-03-15"),
  //                                           "o_orderdate"_))),
  //                   "Where"_("Equal"_("c_custkey"_, "o_custkey"_))),
  //           "As"_("o_orderkey"_, "o_orderkey"_, "o_orderdate"_,
  //           "o_orderdate"_,
  //                 "o_custkey"_, "o_custkey"_, "o_shippriority"_,
  //                 "o_shippriority"_)),
  //       "By"_("o_custkey"_, "o_orderkey"_, "o_orderdate"_,
  //       "o_shippriority"_)));

  //   CHECK(result ==
  //         "List"_("List"_(34850.16 * 0.05 + 25284.00 * 0.06))); // NOLINT
  // }
}

TEST_CASE("SELECT", "[basics]") { // NOLINT
  boss::engines::numpy::Engine engine;
  auto eval = [&engine](boss::Expression &&expression) mutable {
    return engine.evaluate(std::move(expression));
  };

#ifdef USE_NEW_TABLE_FORMAT
  auto table1 = "Table"_("key"_(createSpansInt(1, 2, 3)),
                         "payload"_(createSpansInt(4, 5, 6)));
#else
  auto table1 = "Table"_("Column"_("key"_, createSpansInt(1, 2, 3)),
                         "Column"_("payload"_, createSpansInt(4, 5, 6)));
#endif

  SECTION("Simple_select") {
    auto const &result = eval(
        "Select"_("Project"_(std::move(table1),
                             "As"_("key"_, "key"_, "payload"_, "payload"_)),
                  "Where"_("Greater"_("key"_, 2))));

    CHECK(result == "List"_("List"_(3, 6))); // NOLINT
  }

// #ifdef USE_NEW_TABLE_FORMAT
//   auto table2 = "Table"_("key"_(createSpansInt(1, 2, 3)),
//                          "payload"_(createSpansInt(4, 5, 6)));
// #else
//   auto table2 = "Table"_("Column"_("key"_, createSpansInt(1, 2, 3)),
//                          "Column"_("payload"_, createSpansInt(4, 5, 6)));
// #endif

//   SECTION("Simple_select") {
//     auto const &result = eval(
//         "Select"_("Project"_(std::move(table2),
//                              "As"_("key"_, "key"_, "payload"_, "payload"_)),
//                   "Where"_("Greater"_(2, "key"_))));

//     CHECK(result == "List"_("List"_(1, 4))); // NOLINT
//   }
}

// TEST_CASE("Gather", "[basics]") { // NOLINT
//   boss::engines::numpy::Engine engine;
//   auto eval = [&engine](boss::Expression &&expression) mutable {
//     return engine.evaluate(std::move(expression));
//   };

// #ifdef USE_NEW_TABLE_FORMAT
//   auto table1 = "Table"_("key"_(createSpansInt(1, 2, 3, 4)),
//                          "payload"_(createSpansInt(5, 6, 7, 8)));
// #else
//   auto table1 = "Table"_("Column"_("key"_, createSpansInt(1, 2, 3, 4)),
//                          "Column"_("payload"_, createSpansInt(5, 6, 7, 8)));
// #endif

//   SECTION("Simple_gather") {
//     std::vector<int32_t> indexes = {0, 2, 3};
//     boss::expressions::ExpressionSpanArguments args;
//     args.emplace_back(boss::Span<int32_t>(std::move(std::vector(indexes))));
//     auto projectExpression = boss::expressions::ComplexExpression("Project"_(
//         std::move(table1), "As"_("key"_, "key"_, "payload"_, "payload"_)));
//     boss::expressions::ExpressionArguments subExpressions;
//     subExpressions.emplace_back(std::move(projectExpression));
//     auto gatherExpression = boss::expressions::ComplexExpression(
//         "Gather"_, {}, std::move(subExpressions), std::move(args));

//     auto const &result = eval(std::move(gatherExpression));

//     CHECK(result == "List"_("List"_(1, 3, 4, 5, 7, 8))); // NOLINT
//   }
// }

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