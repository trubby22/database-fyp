#include "../Source/BOSSNumpyEngine.hpp"

#include <chrono>
#include <cmath>

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

static boss::ComplexExpression shallowCopy(boss::ComplexExpression const& e) {
  auto const& head = e.getHead();
  auto const& dynamics = e.getDynamicArguments();
  auto const& spans = e.getSpanArguments();
  boss::ExpressionArguments dynamicsCopy;
  std::transform(dynamics.begin(), dynamics.end(), std::back_inserter(dynamicsCopy),
                 [](auto const& arg) {
                   return std::visit(
                       boss::utilities::overload(
                           [&](boss::ComplexExpression const& expr) -> boss::Expression {
                             return shallowCopy(expr);
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
            // this would still keep const spans for bools, need to fix later
            return SpanType(typedSpan.begin(), typedSpan.size(), []() {});
          } else {
            // force non-const value for now (otherwise expressions cannot be moved)
            auto* ptr = const_cast<T*>(typedSpan.begin()); // NOLINT
            return boss::Span<T>(ptr, typedSpan.size(), []() {});
          }
        },
        span);
  });
  return {head, {}, std::move(dynamicsCopy), std::move(spansCopy)};
}

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

ComplexExpression boss_to_python(Expression&& rand_table) {
  return "Python"_(
              ""_,
              "Where"_(
                "rand_table"_, move(rand_table)
              )
          );
}

ComplexExpression python_to_boss() {
  return "get_python_var"_(
              "rand_table"_
          );
}

void print_elapsed_time(chrono::nanoseconds elapsed_time) {
  cout << "elapsed time = " << chrono::duration_cast<chrono::seconds>(elapsed_time).count() << "[s]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::milliseconds>(elapsed_time).count() << "[ms]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::microseconds>(elapsed_time).count() << "[µs]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::nanoseconds> (elapsed_time).count() << "[ns]" << endl;
}

int main(int argc, char *argv[]) {
  Engine engine;

  int num_columns = 1 << 3;
  // in MB
  int span_size = 1 << 3;
  // in MB
  int chunk_size = num_columns * span_size;

  // in MB
  for(int table_size : std::vector<int>{1, 10, 100, 1000, 10000}) {
    int num_spans_per_column = table_size / chunk_size;
    Expression rand_table = create_random_table(
      num_columns, num_spans_per_column, span_size);
    
    Expression query = boss_to_python(move(rand_table));

    for (int warmup_it = 0; warmup_it < 1; warmup_it++) {
      Expression query_clone = query.clone(CloneReason::FOR_TESTING);
      engine.evaluate(move(query_clone));
    }

    chrono::nanoseconds total_time = chrono::nanoseconds::zero();
    int num_test_iterations = 3;

    for (int test_it = 0; test_it < num_test_iterations; test_it++) {
      Expression query_clone = query.clone(CloneReason::FOR_TESTING);

      chrono::high_resolution_clock::time_point begin = chrono::high_resolution_clock::now();
      engine.evaluate(move(query_clone));
      chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
      auto elapsed_time = chrono::duration_cast<chrono::nanoseconds>(end - begin);
      total_time += elapsed_time;
    }

    chrono::nanoseconds avg_elapsed_time = total_time / num_test_iterations;
    cout << "table size = " << table_size << " MB" << endl;
    print_elapsed_time(avg_elapsed_time);
    cout << endl;
  }

  auto rand_table = create_random_table(2, 2, 2);
  

  return 0;
}
