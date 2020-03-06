#include "plzero.h"

#include "test.h"

namespace
{
  using namespace plzero;

  struct test_io
  {
    test_io(const char * input) :
      input_{ input },
      is_{ input_ }
    {
    }

    std::string input_;
    std::istringstream is_;
    std::ostringstream os_;

    std::string output()
    {
      return os_.str();
    }
  };

  struct test_c_ctx :
    public test_io,
    public compiler_ctx
  {
    test_c_ctx(const char * input) :
      test_io{ input },
      compiler_ctx{ is_, os_ }
    {
    }
  };

  TEST(plzero__error)
  {
    test_c_ctx c_ctx("");
    c_ctx.cc = 2;
    error(c_ctx, 3);
    ASSERT_EQ(std::string(" ****  ^ 3\n"), c_ctx.output());
  }

  TEST(plzero__getch)
  {
    test_c_ctx c_ctx("ab\nc");
    ASSERT_EQ(' ', c_ctx.ch);
    getch(c_ctx); ASSERT_EQ('a', c_ctx.ch);
    getch(c_ctx); ASSERT_EQ('b', c_ctx.ch);
    getch(c_ctx); ASSERT_EQ('\n', c_ctx.ch);
    getch(c_ctx); ASSERT_EQ('c', c_ctx.ch);
    getch(c_ctx); ASSERT_EQ('\n', c_ctx.ch);
    ASSERT_THROW(getch(c_ctx), error_99_exception);
    ASSERT_EQ("    0 ab\n    0 c\n program incomplete", c_ctx.output());
  }

  TEST(plzero__getsym)
  {
    test_c_ctx c_ctx("const a 142 := (");
    ASSERT_EQ(symbol::nul, c_ctx.sym);
    getsym(c_ctx); ASSERT_EQ(symbol::constsym, c_ctx.sym);
    getsym(c_ctx); ASSERT_EQ(symbol::ident, c_ctx.sym);
    ASSERT_EQ(std::string("a"), c_ctx.id);
    getsym(c_ctx); ASSERT_EQ(symbol::number, c_ctx.sym);
    ASSERT_EQ(142, c_ctx.num);
    getsym(c_ctx); ASSERT_EQ(symbol::becomes, c_ctx.sym);
    getsym(c_ctx); ASSERT_EQ(symbol::lparen, c_ctx.sym);
  }

  TEST(plzero__compile_squares)
  {
    test_io io(
"var x, squ;\n\n"
"procedure square;\n"
"begin\n"
"   squ:= x * x\n"
"end;\n"
"\n"
"begin\n"
"   x := 1;\n"
"   while x < 11 do\n"
"   begin\n"
"      call square;\n"
"      x := x + 1\n"
"   end\n"
"end.\n"
    );

    std::string expected{
"    0 var x, squ;\n"
"    1 \n"
"    1 procedure square;\n"
"    1 begin\n"
"    3    squ:= x * x\n"
"    5 end;\n"
"    0  jmp  0    0\n"
"    1  jmp  0    2\n"
"    2  int  0    3\n"
"    3  lod  1    3\n"
"    4  lod  1    3\n"
"    5  opr  0    4\n"
"    6  sto  1    4\n"
"    7  opr  0    0\n"
"    8 \n"
"    8 begin\n"
"    9    x := 1;\n"
"   11    while x < 11 do\n"
"   15    begin\n"
"   15       call square;\n"
"   16       x := x + 1\n"
"   18    end\n"
"   20 end.\n"
"    0  jmp  0    8\n"
"    1  jmp  0    2\n"
"    2  int  0    3\n"
"    3  lod  1    3\n"
"    4  lod  1    3\n"
"    5  opr  0    4\n"
"    6  sto  1    4\n"
"    7  opr  0    0\n"
"    8  int  0    5\n"
"    9  lit  0    1\n"
"   10  sto  0    3\n"
"   11  lod  0    3\n"
"   12  lit  0   11\n"
"   13  opr  0   10\n"
"   14  jpc  0   21\n"
"   15  cal  0    2\n"
"   16  lod  0    3\n"
"   17  lit  0    1\n"
"   18  opr  0    2\n"
"   19  sto  0    3\n"
"   20  jmp  0   11\n"
"   21  opr  0    0\n"
" start pl/0\n"
"1\n"
"1\n"
"2\n"
"4\n"
"3\n"
"9\n"
"4\n"
"16\n"
"5\n"
"25\n"
"6\n"
"36\n"
"7\n"
"49\n"
"8\n"
"64\n"
"9\n"
"81\n"
"10\n"
"100\n"
"11\n"
" end pl/0\n"
    };

    compile(io.is_, io.os_);

    ASSERT_EQ(expected, io.output());
  }

} // anonymous namespace
