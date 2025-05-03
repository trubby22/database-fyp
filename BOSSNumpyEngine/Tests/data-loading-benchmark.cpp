#include "../Source/BOSSNumpyEngine.hpp"

#include <chrono>
#include <cmath>

// #define DEBUG

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

ComplexExpression boss_to_python_matrix(Expression&& rand_table) {
  return "Python"_(
    R"(
import numpy as np

def foo(a):
  print(a)
  print(a.shape)
  print(a.dtype)

table = rand_table["table"]
cols = list(table.values()) # list[list[npy_arr]]
print(cols)
col_0 = cols[0] # list[npy_arr]
print(col_0)
col_0_merged = np.concatenate(col_0) # npy_arr
foo(col_0_merged)

# cols = [np.concatenate(xs) for xs in table.values()]
# mat_helper = np.stack(cols)
# mat = {"table": {}, "matrix": {"data": mat_helper, "col_names": list(table.keys())}}
)"_,
    "Where"_(
                "rand_table"_, move(rand_table)
              )
          );
}

ComplexExpression python_to_boss_matrix() {
  return "get_python_var"_(
              "mat"_
          );
}

#ifdef DEBUG
ComplexExpression python_print() {
  return "Python"_(
              "print(rand_table)"_
          );
} 

ComplexExpression python_print_matrix() {
  return "Python"_(
    R"(
print('table', table, sep='\n')
print('cols', cols, sep='\n')
print('mat_helper', mat_helper, sep='\n')
print('mat', mat, sep='\n')
)"_
  );
}
#endif

void print_elapsed_time(chrono::nanoseconds elapsed_time) {
  cout << "elapsed time = " << chrono::duration_cast<chrono::seconds>(elapsed_time).count() << "[s]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::milliseconds>(elapsed_time).count() << "[ms]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::microseconds>(elapsed_time).count() << "[µs]" << endl;
  cout << "elapsed time = " << chrono::duration_cast<chrono::nanoseconds> (elapsed_time).count() << "[ns]" << endl;
}

void test_round_trip_move(Expression &&rand_table, ull span_size_bytes) {
  Engine engine(span_size_bytes);

  chrono::high_resolution_clock::time_point begin = chrono::high_resolution_clock::now();

  Expression boss_to_python_query = boss_to_python(move(rand_table));
  Expression boss_to_python_result = engine.evaluate(
    move(boss_to_python_query)
  );
#ifdef DEBUG
  engine.evaluate(move(python_print()));
#endif

  Expression query = python_to_boss();
  Expression result = engine.evaluate(move(query));
#ifdef DEBUG
  cout << "boss table from python" << endl;
  cout << result << endl;
  cout << endl;
#endif

  chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
  chrono::nanoseconds elapsed_time = chrono::duration_cast<chrono::nanoseconds>(end - begin);
  cout << "round trip" << endl;
  print_elapsed_time(move(elapsed_time));
  cout << endl;
}

void test_boss_to_python(Expression &&rand_table, ull span_size_bytes) {
  Engine engine(span_size_bytes);

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
  cout << "boss to python" << endl;
  print_elapsed_time(move(avg_elapsed_time));
  cout << endl;
}

void test_python_to_boss(Expression &&rand_table, ull span_size_bytes) {
  Engine engine(span_size_bytes);

  Expression boss_to_python_query = boss_to_python(move(rand_table));
  Expression boss_to_python_result = engine.evaluate(
    move(boss_to_python_query)
  );
#ifdef DEBUG
  engine.evaluate(move(python_print()));
#endif

  Expression query = python_to_boss();

  for (int warmup_it = 0; warmup_it < 1; warmup_it++) {
    Expression query_clone = query.clone(CloneReason::FOR_TESTING);
    Expression result = engine.evaluate(move(query_clone));
#ifdef DEBUG
    cout << "boss table from python" << endl;
    cout << result << endl;
    cout << endl;
#endif
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
  cout << "python to boss" << endl;
  print_elapsed_time(move(avg_elapsed_time));
  cout << endl;
}

void test_boss_to_python_matrix(Expression &&rand_table, ull span_size_bytes) {
  Engine engine(span_size_bytes);

  Expression query = boss_to_python_matrix(move(rand_table));

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
  cout << "boss to python matrix" << endl;
  print_elapsed_time(move(avg_elapsed_time));
  cout << endl;
}

void test_python_to_boss_matrix(Expression &&rand_table, ull span_size_bytes) {
  Engine engine(span_size_bytes);

  Expression boss_to_python_query = boss_to_python_matrix(move(rand_table));
  Expression boss_to_python_result = engine.evaluate(
    move(boss_to_python_query)
  );
#ifdef DEBUG
  engine.evaluate(move(python_print_matrix()));
#endif

  Expression query = python_to_boss_matrix();

  for (int warmup_it = 0; warmup_it < 1; warmup_it++) {
    Expression query_clone = query.clone(CloneReason::FOR_TESTING);
    Expression result = engine.evaluate(move(query_clone));
#ifdef DEBUG
    cout << "boss table from python matrix" << endl;
    cout << result << endl;
    cout << endl;
#endif
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
  cout << "python to boss matrix" << endl;
  print_elapsed_time(move(avg_elapsed_time));
  cout << endl;
}

int main(int argc, char *argv[]) {
  ull num_columns = 4;
  // ull num_columns = 1;

  // in B
  for(ull table_size : std::vector<ull>{
    // static_cast<ull>(1ULL << 20), 
    // static_cast<ull>(10ULL << 20), 
    // static_cast<ull>(100ULL << 20), 
    // static_cast<ull>(1ULL << 30), 
    static_cast<ull>(5ULL << 30),
    // static_cast<ull>(10ULL << 30), 
    // static_cast<ull>(100ULL << 30)
    }) {
  // for(ull table_size : std::vector<ull>{1000 << 20, 10000 << 20}) {
  // for(ull table_size : std::vector<ull>{1 << 20, 10 << 20}) {
    // in B
    for (ull span_size_bytes : std::vector<ull>{1ULL << 3 << 20}) {
      // in B
      if (table_size < (1ULL << 30)) {
        cout << "table size = " << (table_size >> 20) << " MB" << endl;
      } else {
        cout << "table size = " << (table_size >> 30) << " GB" << endl;
      }
      cout << endl;

      vector<unique_ptr<vector<int>>> span_ptrs;

      chrono::high_resolution_clock::time_point begin = chrono::high_resolution_clock::now();
      Expression rand_table = create_random_table(
        num_columns, table_size, span_size_bytes, span_ptrs);
      chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
      auto elapsed_time = chrono::duration_cast<chrono::nanoseconds>(end - begin);
      cout << "create random table" << endl;
      print_elapsed_time(move(elapsed_time));
      cout << endl;
#ifdef DEBUG
      cout << "rand_table" << endl;
      cout << rand_table << endl;
#endif

      test_round_trip_move(move(rand_table), span_size_bytes);

      // Expression rand_table_clone = rand_table.clone(CloneReason::FOR_TESTING);

      // test_boss_to_python(move(rand_table_clone), span_size_bytes);
      // rand_table_clone = rand_table.clone(CloneReason::FOR_TESTING);
      // test_python_to_boss(move(rand_table_clone), span_size_bytes);
      // rand_table_clone = rand_table.clone(CloneReason::FOR_TESTING);
      // test_boss_to_python_matrix(move(rand_table_clone), span_size_bytes);
      // rand_table_clone = rand_table.clone(CloneReason::FOR_TESTING);
      // test_python_to_boss_matrix(move(rand_table_clone), span_size_bytes);
    }
  }

  return 0;
}
