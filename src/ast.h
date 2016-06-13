#pragma once

#include "optional.h"
#include "variant.h"

namespace ast
{
  struct expression;

  struct factor
  {
    variant::ptr<int, expression> value;
  };

  enum class factor_operation
  {
    multiplication,
    division
  };

  struct factor_node
  {
    factor_operation op;
    factor value;
    optional::ptr<factor_node> next;
  };

  struct term
  {
    factor first_factor;
    optional::ptr<factor_node> next_factor;
  };

  enum class term_operation
  {
    addition,
    substraction
  };

  struct term_node
  {
    term_operation op;
    term value;
    optional::ptr<term_node> next;
  };

  struct expression
  {
    bool negate_first;
    term first_term;
    optional::ptr<term_node> next_term;
  };
} // namespace ast
