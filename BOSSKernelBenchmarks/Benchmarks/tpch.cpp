#include "config.hpp"
#include "dataGeneration.cpp"
#include "utilities.cpp"
#include <benchmark/benchmark.h>
#include <iostream>

using SpanArguments = boss::DefaultExpressionSystem::ExpressionSpanArguments;
using SpanArgument = boss::DefaultExpressionSystem::ExpressionSpanArgument;
using ComplexExpression = boss::DefaultExpressionSystem::ComplexExpression;
using ExpressionArguments = boss::ExpressionArguments;

using namespace std;
using intType = int32_t;
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


