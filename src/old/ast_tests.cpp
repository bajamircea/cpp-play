#include "ast.h"

#include "test.h"

namespace
{
  using namespace ast;

  TEST(ast__construct_simple_tree)
  {
    // 1 + 2
    factor factor_two{ variant::ptr<int, ast::expression>{ 2 } };

    expression simple
    {
      false,
      term
      {
        factor{ variant::ptr<int, ast::expression>{ 1 } },
        {}
      },
      optional::ptr<term_node>
      {
        term_node
        {
          term_operation::addition,
          term
          {
            factor{ variant::ptr<int, ast::expression>{ 2 } },
            {}
          },
          {}
        }
      }
    };
  }
} // anonymous namespace
