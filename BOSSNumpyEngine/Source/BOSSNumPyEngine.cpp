#include "BOSSNumPyEngine.hpp"

using namespace std;

using string_literals::operator""s;
using boss::utilities::operator""_;
using boss::ComplexExpression;
using boss::Span;
using boss::Symbol;

using boss::Expression;

namespace boss::engines::numpy {

Expression Engine::evaluate(Expression &&e) {
  cout << e << endl;
  return move(e);
};

} // namespace boss::engines::numpy

static auto& enginePtr(bool initialise = true) {
  static mutex m;
  lock_guard const lock(m);
  static auto engine = unique_ptr<boss::engines::numpy::Engine>();
  if(!engine && initialise) {
    engine.reset(new boss::engines::numpy::Engine());
  }
  return engine;
}

extern "C" BOSSExpression* evaluate(BOSSExpression* e) {
  auto* r = new BOSSExpression{enginePtr()->evaluate(move(e->delegate))};
  return r;
}

extern "C" void reset() { enginePtr(false).reset(nullptr); }
