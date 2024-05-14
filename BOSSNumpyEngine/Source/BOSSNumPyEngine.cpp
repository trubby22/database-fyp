#include "BOSSNumPyEngine.hpp"

using namespace std;

using string_literals::operator"" s;
using boss::utilities::operator"" _;
using boss::ComplexExpression;
using boss::Span;
using boss::Symbol;

using boss::Expression;

namespace boss::engines::numpy
{

  Expression
  Engine::evaluate(Expression &&e)
  {
    cout << e << endl;

    return std::visit(
        boss::utilities::overload(
            [this](ComplexExpression &&expression) -> boss::Expression
            {
              auto [head, statics, dynamics, spans] = std::move(expression).decompose();

              cout << head.getName() << endl;
              for (auto &&arg : dynamics)
              {
                cout << arg << endl;
              }
              for (auto &&arg : spans) 
              {
                evaluate(arg);
              }

              if (head == "Project" _)
              {
                return std::move(expression);
              }
              else if (head == "Select" _)
              {
                return std::move(expression);
              }
              else if (head == "Join" _)
              {
                return std::move(expression);
              }
              else if (head == "Group" _)
              {
                return std::move(expression);
              }
              else if (head == "As" _)
              {
                return std::move(expression);
              }
              else if (head == "Where" _)
              {
                return std::move(expression);
              }
              else if (head == "And" _)
              {
                return std::move(expression);
              }
              else if (head == "Sum" _)
              {
                return std::move(expression);
              }
              else if (head == "Times" _)
              {
                return std::move(expression);
              }
              else if (head == "Greater" _)
              {
                return std::move(expression);
              }
              else if (head == "Table" _)
              {
                return std::move(expression);
              }
              else if (head == "List" _)
              {
                return std::move(expression);
              }
              std::transform(std::make_move_iterator(dynamics.begin()),
                             std::make_move_iterator(dynamics.end()),
                             dynamics.begin(), [this](auto &&arg)
                             { return evaluate(
                                   std::forward<decltype(arg)>(arg)); });
              return boss::ComplexExpression(
                  std::move(head), {}, std::move(dynamics), std::move(spans));
            },
            [this](Symbol &&symbol) -> boss::Expression
            {
              return std::move(symbol);
            },
            [](auto &&arg) -> boss::Expression
            {
              return std::forward<decltype(arg)>(arg);
            }),
        std::move(e));
  };

} // namespace boss::engines::numpy

static auto &
enginePtr(bool initialise = true)
{
  static auto engine = std::unique_ptr<boss::engines::numpy::Engine>();
  if (!engine && initialise)
  {
    engine.reset(new boss::engines::numpy::Engine());
  }
  return engine;
}

extern "C" BOSSExpression *
evaluate(BOSSExpression *e)
{
  static std::mutex m;
  std::lock_guard lock(m);
  auto *r = new BOSSExpression{enginePtr()->evaluate(std::move(e->delegate))};
  return r;
};

extern "C" void
reset()
{
  enginePtr(false).reset(nullptr);
}
