#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <map>
#include <vector>

namespace plzero
{
  struct error_99_exception { };

  constexpr int LEVMAX = 3; // max depth of block nesting
  constexpr unsigned int CXMAX = 200; // max size of code array

  enum class symbol
  {
    nul, ident, number, plus, minus, times, slash, oddsym,
    eql, neq, lss, leq, gtr, geq, lparen, rparen, comma, semicolon,
    period, becomes, beginsym, endsym, ifsym, thensym,
    whilesym, dosym, callsym, constsym, varsym, procsym
  };
  using alfa = std::string;
  enum class obj { constant, varible, proc };
  using symset = std::set<symbol>;
  enum class fct { lit, opr, lod, sto, cal, intfct, jmp, jpc };
  enum class fct_opr {
    ret, neg, add, sub, mul, div, odd,
    dummy_seven,
    eql, neq, lss, geq, gtr, leq
  };
  struct instruction
  {
    fct f; // function code
    int l; // level
    int a; // address
  };
  struct table_elem
  {
    obj kind;
    alfa name;
    unsigned int level;
    int adr_or_val;
  };

  struct compiler_ctx
  {
    compiler_ctx(std::istream & input, std::ostream & output) :
      in{ input }, out{ output }
    {
    }

    std::istream & in;
    std::ostream & out;
    char ch{' '}; // last char read
    symbol sym{ symbol::nul }; // last symbol read
    alfa id; // last identifier
    int num{ 0 }; // last number read
    unsigned int cc{ 0 }; // character count
    unsigned err{ 0 };
    std::string line; // curent line
    alfa a;
    std::vector<instruction> code;
    std::map<std::string, symbol> wsym{
      {"begin", symbol::beginsym},
      {"call", symbol::callsym},
      {"const", symbol::constsym},
      {"do", symbol::dosym},
      {"end", symbol::endsym},
      {"if", symbol::ifsym},
      {"odd", symbol::oddsym},
      {"procedure", symbol::procsym},
      {"then", symbol::thensym},
      {"var", symbol::varsym},
      {"while", symbol::whilesym},
    };
    std::map<char, symbol> ssym{
      {'+', symbol::plus},
      {'-', symbol::minus},
      {'*', symbol::times},
      {'/', symbol::slash},
      {'(', symbol::lparen},
      {')', symbol::rparen},
      {'=', symbol::eql},
      {',', symbol::comma},
      {'.', symbol::period},
      {'#', symbol::neq},
      {'<', symbol::lss},
      {'>', symbol::gtr},
      {'[', symbol::leq},
      {']', symbol::geq},
      {';', symbol::semicolon},
    };
    std::map<fct, std::string> mnemonic{
      {fct::lit, "  lit"},
      {fct::opr, "  opr"},
      {fct::lod, "  lod"},
      {fct::sto, "  sto"},
      {fct::cal, "  cal"},
      {fct::intfct, "  int"},
      {fct::jmp, "  jmp"},
      {fct::jpc, "  jpc"},
    };
    symset declbegsys{
      symbol::constsym, symbol::varsym, symbol::procsym
    };
    symset statbegsys{
      symbol::beginsym, symbol::callsym, symbol::ifsym, symbol::whilesym
    };
    symset facbegsys{
      symbol::ident, symbol::number, symbol::lparen
    };
    std::vector<table_elem> table;
  };

  inline void error(compiler_ctx & c_ctx, int n)
  {
    c_ctx.out << " ****" << std::string(c_ctx.cc, ' ') << '^'
      << std::setw(2) << n << '\n';
    c_ctx.err += 1;
  }

  inline void getch(compiler_ctx & c_ctx)
  {
    if ((c_ctx.cc + 1) >= c_ctx.line.size())
    {
      if (!std::getline(c_ctx.in, c_ctx.line))
      {
        c_ctx.out << " program incomplete";
        throw error_99_exception();
      }
      c_ctx.cc = 0;
      c_ctx.line += '\n';
      c_ctx.out << std::setw(5) << c_ctx.code.size() << " " << c_ctx.line;
    }
    else
    {
      c_ctx.cc += 1;
    }
    c_ctx.ch = c_ctx.line[c_ctx.cc];
  }

  inline bool is_whitespace(char ch)
  {
    switch(ch)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      return true;
    }
    return false;
  }

  inline bool is_alpha(char ch)
  {
    return (ch >= 'a') && (ch <= 'z');
  }

  inline bool is_num(char ch)
  {
    return (ch >= '0') && (ch <= '9');
  }

  inline bool is_alphanum(char ch)
  {
    return is_alpha(ch) || is_num(ch);
  }

  inline void getsym(compiler_ctx & c_ctx)
  {
    while (is_whitespace(c_ctx.ch))
    {
      getch(c_ctx);
    }
    if (is_alpha(c_ctx.ch))
    {
      c_ctx.id.clear();
      do
      {
        c_ctx.id += c_ctx.ch;
        getch(c_ctx);
      } while (is_alphanum(c_ctx.ch));
      auto it = c_ctx.wsym.find(c_ctx.id);
      c_ctx.sym = (it == c_ctx.wsym.end()) ? symbol::ident : it->second;
    }
    else if (is_num(c_ctx.ch))
    {
      c_ctx.num = 0;
      int digits = 0;
      do
      {
        c_ctx.num = c_ctx.num * 10 + (c_ctx.ch - '0');
        ++digits;
        getch(c_ctx);
      } while (is_num(c_ctx.ch));
      if (digits > 9)
      {
        error(c_ctx, 30);
      }
      c_ctx.sym = symbol::number;
    }
    else if (c_ctx.ch == ':')
    {
      getch(c_ctx);
      c_ctx.sym = (c_ctx.ch == '=') ? symbol::becomes : symbol::nul;
      getch(c_ctx);
    }
    else
    {
      auto it = c_ctx.ssym.find(c_ctx.ch);
      c_ctx.sym = (it == c_ctx.ssym.end()) ? symbol::nul : it->second;
      getch(c_ctx);
    }
  }

  void gen(compiler_ctx & c_ctx, fct f, int l, int a)
  {
    if (c_ctx.code.size() > CXMAX)
    {
        c_ctx.out << " program too long";
        throw error_99_exception();
    }
    c_ctx.code.push_back({ f, l, a });
  }

  void gen(compiler_ctx & c_ctx, fct_opr o)
  {
    gen(c_ctx, fct::opr, 0, static_cast<int>(o));
  }

  void test(compiler_ctx & c_ctx, const symset & s1, const symset & s2, int n)
  {
    if (s1.end() != s1.find(c_ctx.sym))
    {
      return;
    }
    error(c_ctx, n);
    while((s1.end() == s1.find(c_ctx.sym)) &&
      (s2.end() == s2.find(c_ctx.sym)))
    {
      getsym(c_ctx);
    }
  }

  void listcode(compiler_ctx & c_ctx, unsigned int from)
  {
    for (unsigned int i = from; i < c_ctx.code.size(); ++i)
    {
      const auto & instr = c_ctx.code[i];
      c_ctx.out << std::setw(5) << i
        << c_ctx.mnemonic[instr.f]
        << std::setw(3) << instr.l
        << std::setw(5) << instr.a
        << '\n';
    }
  }

  struct block_ctx
  {
    unsigned int lev;
    int dx{ 3 }; // data allocation index
  };

  table_elem * find_id(compiler_ctx & c_ctx, const alfa & id)
  {
    auto & t = c_ctx.table;
    auto it = std::find_if(std::rbegin(t), std::rend(t),
      [&id](const table_elem & e) { return id == e.name; });
    if (std::rend(t) == it)
    {
      return nullptr;
    }
    return &(*it);
  }

  void constdeclaration(compiler_ctx & c_ctx, block_ctx & b_ctx)
  {
    if (c_ctx.sym == symbol::ident)
    {
      getsym(c_ctx);
      switch(c_ctx.sym)
      {
      case symbol::eql:
      case symbol::becomes:
        if (c_ctx.sym == symbol::becomes)
        {
          error(c_ctx, 1);
        }
        getsym(c_ctx);
        if (c_ctx.sym == symbol::number)
        {
          c_ctx.table.push_back(
            { obj::constant, c_ctx.id, b_ctx.lev, c_ctx.num });
          getsym(c_ctx);
        }
        else
        {
          error(c_ctx, 2);
        }
        break;
      default:
        error(c_ctx, 3);
      }
    }
    else
    {
      error(c_ctx, 4);
    }
  }

  void vardeclaration(compiler_ctx & c_ctx, block_ctx & b_ctx)
  {
    if (c_ctx.sym == symbol::ident)
    {
      c_ctx.table.push_back(
        { obj::varible, c_ctx.id, b_ctx.lev, b_ctx.dx });
      b_ctx.dx += 1;
      getsym(c_ctx);
    }
    else
    {
      error(c_ctx, 4);
    }
  }

  //forward declaration
  void expression(compiler_ctx & c_ctx, block_ctx & b_ctx, const symset & fsys);

  void factor(compiler_ctx & c_ctx, block_ctx & b_ctx, const symset & fsys)
  {
    test(c_ctx, c_ctx.facbegsys, fsys, 24);
    while (c_ctx.facbegsys.cend() != c_ctx.facbegsys.find(c_ctx.sym))
    {
      switch(c_ctx.sym)
      {
        case symbol::ident:
          {
            table_elem * pte = find_id(c_ctx, c_ctx.id);
            if (nullptr == pte)
            {
              error(c_ctx, 11);
            }
            else
            {
              table_elem & te = *pte;
              switch (te.kind)
              {
                case obj::constant:
                  gen(c_ctx, fct::lit, 0, te.adr_or_val);
                  break;
                case obj::varible:
                  gen(c_ctx, fct::lod, (int)(b_ctx.lev - te.level), te.adr_or_val);
                  break;
                case obj::proc:
                  error(c_ctx, 21);
                  break;
              }
            }
            getsym(c_ctx);
          }
          break;
        case symbol::number:
          gen(c_ctx, fct::lit, 0, c_ctx.num);
          getsym(c_ctx);
          break;
        case symbol::lparen:
          getsym(c_ctx);
          {
            symset s;
            s.insert(symbol::rparen);
            s.insert(fsys.begin(), fsys.end());
            expression(c_ctx, b_ctx, s);
          }
          if (c_ctx.sym == symbol::rparen)
          {
            getsym(c_ctx);
          }
          else
          {
            error(c_ctx, 22);
          }
          test(c_ctx, c_ctx.facbegsys, symset{ symbol::lparen }, 23);
          break;
        default:
          throw std::logic_error("unhandled symbol");
      }
    }
  }

  void term(compiler_ctx & c_ctx, block_ctx & b_ctx, const symset & fsys)
  {
    symset s;
    s.insert(fsys.begin(), fsys.end());
    s.insert(symbol::times);
    s.insert(symbol::slash);
    factor(c_ctx, b_ctx, s);
    while ((c_ctx.sym == symbol::times) ||
      (c_ctx.sym == symbol::slash))
    {
      symbol mulop = c_ctx.sym;
      getsym(c_ctx);
      factor(c_ctx, b_ctx, s);
      if (mulop == symbol::times)
      {
        gen(c_ctx, fct_opr::mul);
      }
      else
      {
        gen(c_ctx, fct_opr::div);
      }
    }
  }

  void expression(compiler_ctx & c_ctx, block_ctx & b_ctx, const symset & fsys)
  {
    symset s;
    s.insert(fsys.begin(), fsys.end());
    s.insert(symbol::plus);
    s.insert(symbol::minus);
    if ((c_ctx.sym == symbol::plus) ||
      (c_ctx.sym == symbol::minus))
    {
      symbol addop = c_ctx.sym;
      getsym(c_ctx);
      term(c_ctx, b_ctx, s);
      if (addop == symbol::minus)
      {
        gen(c_ctx, fct_opr::neg);
      }
    }
    else
    {
      term(c_ctx, b_ctx, s);
    }
    while ((c_ctx.sym == symbol::plus) ||
      (c_ctx.sym == symbol::minus))
    {
      symbol addop = c_ctx.sym;
      getsym(c_ctx);
      term(c_ctx, b_ctx, s);
      if (addop == symbol::plus)
      {
        gen(c_ctx, fct_opr::add);
      }
      else
      {
        gen(c_ctx, fct_opr::sub);
      }
    }
  }

  void condition(compiler_ctx & c_ctx, block_ctx & b_ctx, const symset & fsys)
  {
    if (c_ctx.sym == symbol::oddsym)
    {
      getsym(c_ctx);
      expression(c_ctx, b_ctx, fsys);
      gen(c_ctx, fct_opr::odd);
    }
    else
    {
      symset sop{
        symbol::eql, symbol::neq,
        symbol::lss, symbol::leq,
        symbol::gtr, symbol::geq,
      };

      symset s;
      s.insert(sop.begin(), sop.end());
      s.insert(fsys.begin(), fsys.end());

      expression(c_ctx, b_ctx, s);

      if (sop.cend() == sop.find(c_ctx.sym))
      {
        error(c_ctx, 22);
      }
      else
      {
        symbol relop = c_ctx.sym;
        getsym(c_ctx);
        expression(c_ctx, b_ctx, s);
        switch(relop)
        {
          case symbol::eql:
            gen(c_ctx, fct_opr::eql);
            break;
          case symbol::neq:
            gen(c_ctx, fct_opr::neq);
            break;
          case symbol::lss:
            gen(c_ctx, fct_opr::lss);
            break;
          case symbol::leq:
            gen(c_ctx, fct_opr::leq);
            break;
          case symbol::gtr:
            gen(c_ctx, fct_opr::gtr);
            break;
          case symbol::geq:
            gen(c_ctx, fct_opr::geq);
            break;
          default:
            throw std::logic_error("unhandled symbol");
        }
      }
    }
  }


  void statement(compiler_ctx & c_ctx, block_ctx & b_ctx, const symset & fsys)
  {
    switch (c_ctx.sym)
    {
      case symbol::ident:
        {
          table_elem * pte = find_id(c_ctx, c_ctx.id);
          if (nullptr == pte)
          {
            error(c_ctx, 11);
          }
          else
          {
            table_elem & te = *pte;
            if (te.kind != obj::varible)
            {
              error(c_ctx, 12);
            }
            getsym(c_ctx);
            if (c_ctx.sym == symbol::becomes)
            {
              getsym(c_ctx);
            }
            else
            {
              error(c_ctx, 13);
            }
            expression(c_ctx, b_ctx, fsys);
            if (te.kind == obj::varible)
            {
              gen(c_ctx, fct::sto, (int)(b_ctx.lev - te.level), te.adr_or_val);
            }
          }
        }
        break;
      case symbol::callsym:
        {
          getsym(c_ctx);
          if (c_ctx.sym != symbol::ident)
          {
            error(c_ctx, 14);
          }
          else
          {
            table_elem * pte = find_id(c_ctx, c_ctx.id);
            if (nullptr == pte)
            {
              error(c_ctx, 11);
            }
            else
            {
              table_elem & te = *pte;
              if (te.kind == obj::proc)
              {
                gen(c_ctx, fct::cal, (int)(b_ctx.lev - te.level), te.adr_or_val);
              }
              else
              {
                error(c_ctx, 15);
              }
              getsym(c_ctx);
            }
          }
        }
        break;
      case symbol::ifsym:
        {
          getsym(c_ctx);
          symset s;
          s.insert(symbol::thensym);
          s.insert(symbol::dosym);
          s.insert(fsys.begin(), fsys.end());

          condition(c_ctx, b_ctx, s);
          if (c_ctx.sym == symbol::thensym)
          {
            getsym(c_ctx);
          }
          else
          {
            error(c_ctx, 16);
          }
          size_t cx1 = c_ctx.code.size();
          gen(c_ctx, fct::jpc, 0, 0);
          statement(c_ctx, b_ctx, s);
          c_ctx.code[cx1].a = (int)(c_ctx.code.size());
        }
        break;
      case symbol::beginsym:
        {
          getsym(c_ctx);

          symset s;
          s.insert(symbol::semicolon);
          s.insert(symbol::endsym);
          s.insert(fsys.begin(), fsys.end());

          statement(c_ctx, b_ctx, s);
          while ((c_ctx.sym == symbol::semicolon) ||
            (c_ctx.statbegsys.cend() != c_ctx.statbegsys.find(c_ctx.sym)))
          {
            if (c_ctx.sym == symbol::semicolon)
            {
              getsym(c_ctx);
            }
            else
            {
              error(c_ctx, 10);
            }
            statement(c_ctx, b_ctx, s);
          }
          if (c_ctx.sym == symbol::endsym)
          {
            getsym(c_ctx);
          }
          else
          {
            error(c_ctx, 17);
          }
        }
        break;
      case symbol::whilesym:
        {
          size_t cx1 = c_ctx.code.size();
          getsym(c_ctx);
          symset s;
          s.insert(symbol::dosym);
          s.insert(fsys.begin(), fsys.end());
          condition(c_ctx, b_ctx, s);
          size_t cx2 = c_ctx.code.size();
          gen(c_ctx, fct::jpc, 0, 0);
          if (c_ctx.sym == symbol::dosym)
          {
            getsym(c_ctx);
          }
          else
          {
            error(c_ctx, 18);
          }
          statement(c_ctx, b_ctx, fsys);
          gen(c_ctx, fct::jmp, 0, (int)cx1);
          c_ctx.code[cx2].a = (int)(c_ctx.code.size());
        }
        break;
      default:
        throw std::logic_error("unhandled symbol");
    }
    test(c_ctx, fsys, symset(), 19);
  }

  void block(compiler_ctx & c_ctx, unsigned int lev, const symset & fsys)
  {
    block_ctx b_ctx;
    b_ctx.lev = lev;
    unsigned int tx0 = static_cast<unsigned int>(c_ctx.table.size());
    unsigned int block_jmp_addr = static_cast<unsigned int>(c_ctx.code.size());
    gen(c_ctx, fct::jmp, 0, 0);
    if (lev > LEVMAX)
    {
      error(c_ctx, 32);
    }
    do
    {
      if (c_ctx.sym == symbol::constsym)
      {
        getsym(c_ctx);
        do
        {
          constdeclaration(c_ctx, b_ctx);
          while (c_ctx.sym == symbol::comma)
          {
            getsym(c_ctx);
            constdeclaration(c_ctx, b_ctx);
          }
          if (c_ctx.sym == symbol::semicolon)
          {
            getsym(c_ctx);
          }
          else
          {
            error(c_ctx, 5);
          }
        } while (c_ctx.sym == symbol::ident);
      }
      if (c_ctx.sym == symbol::varsym)
      {
        getsym(c_ctx);
        do
        {
          vardeclaration(c_ctx, b_ctx);
          while (c_ctx.sym == symbol::comma)
          {
            getsym(c_ctx);
            vardeclaration(c_ctx, b_ctx);
          }
          if (c_ctx.sym == symbol::semicolon)
          {
            getsym(c_ctx);
          }
          else
          {
            error(c_ctx, 5);
          }
        } while (c_ctx.sym == symbol::ident);
      }
      while (c_ctx.sym == symbol::procsym)
      {
        getsym(c_ctx);
        if (c_ctx.sym == symbol::ident)
        {
          c_ctx.table.push_back(
            { obj::proc, c_ctx.id, b_ctx.lev, 0 });
          getsym(c_ctx);
        }
        else
        {
          error(c_ctx, 4);
        }
        if (c_ctx.sym == symbol::semicolon)
        {
          getsym(c_ctx);
        }
        else
        {
          error(c_ctx, 5);
        }
        {
          symset s;
          s.insert(fsys.begin(), fsys.end());
          s.insert(symbol::semicolon);
          block(c_ctx, b_ctx.lev + 1, s);
        }
        if (c_ctx.sym == symbol::semicolon)
        {
          getsym(c_ctx);
          symset s;
          s.insert(c_ctx.statbegsys.begin(), c_ctx.statbegsys.end());
          s.insert(symbol::ident);
          s.insert(symbol::procsym);
          test(c_ctx, s, fsys, 6);
        }
        else
        {
          error(c_ctx, 5);
        }
      }
      symset s;
      s.insert(c_ctx.statbegsys.begin(), c_ctx.statbegsys.end());
      s.insert(symbol::ident);
      test(c_ctx, s, c_ctx.declbegsys, 7);
    } while (c_ctx.declbegsys.end() != c_ctx.declbegsys.find(c_ctx.sym));
    c_ctx.code[block_jmp_addr].a = static_cast<int>(c_ctx.code.size());
    if (tx0 > 0)
    {
      c_ctx.table[tx0 - 1].adr_or_val = static_cast<int>(c_ctx.code.size());
    }
    gen(c_ctx, fct::intfct, 0, static_cast<int>(b_ctx.dx));
    {
      symset s;
      s.insert(symbol::semicolon);
      s.insert(symbol::endsym);
      s.insert(fsys.begin(), fsys.end());
      statement(c_ctx, b_ctx, s);
    }
    gen(c_ctx, fct_opr::ret);
    test(c_ctx, fsys, symset(), 8);
    listcode(c_ctx, 0); // cx0
  }

  struct vm_ctx
  {
    vm_ctx(unsigned int stack_size = 500) :
      s(stack_size)
    {
    }

    unsigned int p{ 0 }; // program register
    unsigned int b{ 1 }; // base register
    unsigned int t{ 0 }; // topstack register
    std::vector<int> s; // datastore

    unsigned int get_idx(unsigned int pos) const
    {
      return static_cast<unsigned int>(s[pos]);
    }

    template<typename F>
    void stack_op(F f)
    {
      t -= 1;
      s[t] = f(s[t], s[t + 1]);
    }

    template<typename P>
    void stack_pred(P pred)
    {
      t -= 1;
      s[t] = pred(s[t], s[t + 1]) ? 1 : 0;
    }
  };

  unsigned int base(const vm_ctx & v_ctx, int l)
  {
    unsigned int b1{ v_ctx.b };
    for ( ; l > 0 ; --l)
    {
      b1 = v_ctx.get_idx(b1);
    }
    return b1;
  }

  void interpret(const compiler_ctx & c_ctx, vm_ctx & v_ctx)
  {
    c_ctx.out << " start pl/0\n";
    do
    {
      instruction i{ c_ctx.code[v_ctx.p] };
      v_ctx.p += 1;

      switch (i.f)
      {
      case fct::lit:
        v_ctx.t += 1;
        v_ctx.s[v_ctx.t] = i.a;
        break;
      case fct::opr:
        switch(static_cast<fct_opr>(i.a)) // operator
        {
          case fct_opr::ret: // return
            v_ctx.t = v_ctx.b - 1;
            v_ctx.b = v_ctx.get_idx(v_ctx.t + 2);
            v_ctx.p = v_ctx.get_idx(v_ctx.t + 3);
            break;
          case fct_opr::neg:
            v_ctx.s[v_ctx.t] = -v_ctx.s[v_ctx.t];
            break;
          case fct_opr::add:
            v_ctx.stack_op([](int a, int b){ return a + b;});
            break;
          case fct_opr::sub:
            v_ctx.stack_op([](int a, int b){ return a - b;});
            break;
          case fct_opr::mul:
            v_ctx.stack_op([](int a, int b){ return a * b;});
            break;
          case fct_opr::div:
            v_ctx.stack_op([](int a, int b){ return a / b;});
            break;
          case fct_opr::odd:
            v_ctx.s[v_ctx.t] = v_ctx.s[v_ctx.t] & 0x1;
            break;
          case fct_opr::eql:
            v_ctx.stack_pred([](int a, int b){ return a == b;});
            break;
          case fct_opr::neq:
            v_ctx.stack_pred([](int a, int b){ return a != b;});
            break;
          case fct_opr::lss:
            v_ctx.stack_pred([](int a, int b){ return a < b;});
            break;
          case fct_opr::leq:
            v_ctx.stack_pred([](int a, int b){ return a <= b;});
            break;
          case fct_opr::gtr:
            v_ctx.stack_pred([](int a, int b){ return a > b;});
            break;
          case fct_opr::geq:
            v_ctx.stack_pred([](int a, int b){ return a >= b;});
            break;
          default:
            throw std::logic_error("unhandled operation");
        }
        break;
      case fct::lod:
        v_ctx.t += 1;
        v_ctx.s[v_ctx.t] =
          v_ctx.s[base(v_ctx, i.l) + static_cast<unsigned int>(i.a)];
        break;
      case fct::sto:
        v_ctx.s[base(v_ctx, i.l) + static_cast<unsigned int>(i.a)] =
          v_ctx.s[v_ctx.t];
        c_ctx.out << v_ctx.s[v_ctx.t] << '\n';
        v_ctx.t -= 1;
        break;
      case fct::cal: // generate new block mark
        v_ctx.s[v_ctx.t + 1] = static_cast<int>(base(v_ctx, i.l));
        v_ctx.s[v_ctx.t + 2] = static_cast<int>(v_ctx.b);
        v_ctx.s[v_ctx.t + 3] = static_cast<int>(v_ctx.p);
        v_ctx.b = v_ctx.t + 1;
        v_ctx.p = static_cast<unsigned int>(i.a);
        break;
      case fct::intfct:
        v_ctx.t += static_cast<unsigned int>(i.a);
        break;
      case fct::jmp:
        v_ctx.p = static_cast<unsigned int>(i.a);
        break;
      case fct::jpc:
        if (0 == v_ctx.s[v_ctx.t])
        {
          v_ctx.p = static_cast<unsigned int>(i.a);
        }
        v_ctx.t -= 1;
        break;
      }
    } while(0 != v_ctx.p);
    c_ctx.out << " end pl/0";
  }

  void compile(std::istream & in, std::ostream & out)
  {
    compiler_ctx c_ctx{ in, out};
    try
    {
      getsym(c_ctx);
      {
        symset s;
        s.insert(symbol::period);
        s.insert(c_ctx.declbegsys.begin(), c_ctx.declbegsys.end());
        s.insert(c_ctx.statbegsys.begin(), c_ctx.statbegsys.end());

        block(c_ctx, 0, s);
      }
      if (c_ctx.sym != symbol::period)
      {
        error(c_ctx, 9);
      }

      if (c_ctx.err == 0)
      {
        vm_ctx v_ctx;
        interpret(c_ctx, v_ctx);
      }
      else
      {
        c_ctx.out << " errors in pl/0 program";
      }
    }
    catch(const std::logic_error & e)
    {
    }
    catch(const error_99_exception &)
    {
    }
    c_ctx.out << '\n';
  }

} // namespace plzero
