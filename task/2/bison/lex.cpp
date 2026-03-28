#include "lex.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

namespace lex {

G g;

int
come_line(const char* yytext, int yyleng, int yylineno)
{
  (void)yylineno;
  std::string line(yytext, static_cast<std::size_t>(yyleng));
  if (!line.empty() && line.back() == '\n')
    line.pop_back();

  auto blank = line.find(' ');
  if (blank == std::string::npos)
    return YYUNDEF;
  auto q1 = line.find('\'', blank + 1);
  if (q1 == std::string::npos)
    return YYUNDEF;
  auto q2 = line.rfind('\'');
  if (q2 == std::string::npos || q2 <= q1)
    return YYUNDEF;

  auto name = line.substr(0, blank);
  auto value = line.substr(q1 + 1, q2 - q1 - 1);

  static const std::unordered_map<std::string, int> kTokenId = {
    { "identifier", IDENTIFIER },
    { "numeric_constant", CONSTANT },
    { "const", CONST },
    { "int", INT },
    { "void", VOID },
    { "if", IF },
    { "else", ELSE },
    { "while", WHILE },
    { "break", BREAK },
    { "continue", CONTINUE },
    { "return", RETURN },
    { "l_paren", '(' },
    { "r_paren", ')' },
    { "l_brace", '{' },
    { "r_brace", '}' },
    { "semi", ';' },
    { "equal", '=' },
    { "equalequal", EQ_OP },
    { "exclaim", '!' },
    { "exclaimequal", NE_OP },
    { "l_square", '[' },
    { "r_square", ']' },
    { "comma", ',' },
    { "minus", '-' },
    { "plus", '+' },
    { "star", '*' },
    { "slash", '/' },
    { "percent", '%' },
    { "less", '<' },
    { "lessequal", LE_OP },
    { "greater", '>' },
    { "greaterequal", GE_OP },
    { "ampamp", AND_OP },
    { "pipepipe", OR_OP },
    { "eof", YYEOF },
  };

  auto iter = kTokenId.find(name);
  assert(iter != kTokenId.end());

  if (iter->second == IDENTIFIER || iter->second == CONSTANT)
    yylval.RawStr = new std::string(value, value.size());
  return iter->second;
}

int
come(int tokenId, const char* yytext, int yyleng, int yylineno)
{
  g.mId = tokenId;
  g.mText = { yytext, std::size_t(yyleng) };
  g.mLine = yylineno;

  g.mStartOfLine = false;
  g.mLeadingSpace = false;

  return tokenId;
}

} // namespace lex
