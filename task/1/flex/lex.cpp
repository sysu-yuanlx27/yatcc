#include "lex.hpp"
#include <iostream>
#include <cstdio>

void
print_token();

namespace lex {

static const char* kTokenNames[] = {
  "identifier",   "numeric_constant", "string_literal", "int",
  "const",        "void",             "if",             "else",
  "while",        "break",            "continue",       "return",
  "l_brace",      "r_brace",          "l_square",       "r_square",
  "l_paren",      "r_paren",          "semi",           "equal",
  "equalequal",   "exclaim",          "exclaimequal",   "plus",
  "minus",        "star",             "slash",          "percent",
  "less",         "lessequal",        "greater",        "greaterequal",
  "ampamp",       "pipepipe",         "comma"
};

const char*
id2str(Id id)
{
  static char sCharBuf[2] = { 0, 0 };
  if (id == Id::YYEOF) {
    return "eof";
  }
  else if (id < Id::IDENTIFIER) {
    sCharBuf[0] = char(id);
    return sCharBuf;
  }
  return kTokenNames[int(id) - int(Id::IDENTIFIER)];
}

G g;

int
come(int tokenId, const char* yytext, int yyleng, int yylineno)
{
  (void)yylineno;
  g.mId = Id(tokenId);
  g.mText = { yytext, std::size_t(yyleng) };
  if (tokenId == int(Id::YYEOF)) {
    g.mLine = g.mLastTokenLine;
    g.mTokenColumn = g.mLastTokenEndColumn;
    g.mStartOfLine = false;
    g.mLeadingSpace = false;
  }
  else {
    g.mTokenColumn = g.mColumn - yyleng;
    g.mLastTokenLine = g.mLine;
    g.mLastTokenEndColumn = g.mColumn;
  }

  print_token();
  g.mStartOfLine = false;
  g.mLeadingSpace = false;

  return tokenId;
}

void
handleLineDirective(const char* yytext, int yyleng)
{
  (void)yyleng;
  int line = 0;
  char fileBuf[4096] = { 0 };
  if (std::sscanf(yytext, "# %d \"%4095[^\"]\"", &line, fileBuf) == 2) {
    g.mLine = line;
    g.mFile = fileBuf;
  }

  g.mColumn = 1;
  g.mStartOfLine = true;
  g.mLeadingSpace = false;
}

} // namespace lex
