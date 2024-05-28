#include "BOSSNumpyEngine.hpp"

#include <chrono>

using namespace std;
using intType = int32_t;
using boss::engines::numpy::Engine;
using boss::engines::numpy::create_random_table;
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
using boss::expressions::CloneReason;


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

void foo() {
  Engine engine;

  auto lineitem = create_lineitem();

  auto const python_query_result = engine.evaluate(move("Python"_(
    R"(
import numpy as np
table = lineitem["table"]
a = table["l_orderkey"][0]
b = table["l_quantity"][0]
c = np.stack((a, b))
print(c)
print()
d = c @ c.T
print(d)
e = {"table": {}, "matrix": {"data": d, "col_names": ["foo", "bar"]}}
)"_,
    "where"_("lineitem"_, move(lineitem)))));
  cout << "python_query_result " << endl << python_query_result << endl << endl;

  auto const get_matrix_result = engine.evaluate(move("get_python_var"_("e"_
  )));
  cout << "get_matrix_result " << endl << get_matrix_result << endl << endl;
}

void bar() {
  Engine engine;

  auto rand_table = create_random_table(2, 2, 2);

  auto const boss_to_python = engine.evaluate(
    "Python"_(
      R"(
print(foo["table"])
)"_,
      "Where"_(
        "foo"_, move(rand_table)
      )
    )
  );

  auto const python_to_boss = engine.evaluate(
    "get_python_var"_(
      "foo"_
    )
  );

  cout << python_to_boss << endl;
}

void benchmark(Expression &&query) {
  Engine engine;
  int num_warmup = 3;
  int num_main = 10;

  // auto gen = [&query]() {
  //   return query.clone(CloneReason::FOR_TESTING);
  // };
  vector<Expression> vec;
  // generate(begin(vec), end(vec), gen);
  for (int i = 0; i < num_warmup + num_main; i++) {
    vec.emplace_back(query.clone(CloneReason::FOR_TESTING));
  }

  for (int i = 0; i < num_warmup; i++) {
    engine.evaluate(move(vec[i]));
  }

  chrono::steady_clock::time_point begin = chrono::steady_clock::now();
  for (int i = 0; i < num_main; i++) {
    engine.evaluate(move(vec[num_warmup + i]));
  }
  chrono::steady_clock::time_point end = chrono::steady_clock::now();

  cout << "Time difference = " << chrono::duration_cast<chrono::seconds>(end - begin).count() << "[s]" << endl;
  cout << "Time difference = " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << "[ms]" << endl;
  cout << "Time difference = " << chrono::duration_cast<chrono::microseconds>(end - begin).count() << "[µs]" << endl;
  cout << "Time difference = " << chrono::duration_cast<chrono::nanoseconds> (end - begin).count() << "[ns]" << endl;
}

int main(int argc, char *argv[]) {
  auto rand_table = create_random_table(10, 10, 1 << 20);

  benchmark(
    move("Bar"_(
      "Python"_(
      R"(
)"_,
      "Where"_(
        "foo"_, move(rand_table)
      )
    ),
      "get_python_var"_(
      "foo"_
    )
    ))
  );
  return 0;
}
