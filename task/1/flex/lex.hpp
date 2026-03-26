#pragma once

#include <string>
#include <string_view>
#include <cstring>

namespace lex {

enum Id
{
  YYEMPTY = -2,
  YYEOF = 0,     /* "end of file"  */
  YYerror = 256, /* error  */
  YYUNDEF = 257, /* "invalid token"  */
  IDENTIFIER,
  CONSTANT,
  STRING_LITERAL,
  INT,
  CONST,
  VOID,
  IF,
  ELSE,
  WHILE,
  BREAK,
  CONTINUE,
  RETURN,
  L_BRACE,
  R_BRACE,
  L_SQUARE,
  R_SQUARE,
  L_PAREN,
  R_PAREN,
  SEMI,
  EQUAL,
  EQUAL_EQUAL,
  EXCLAIM,
  EXCLAIM_EQUAL,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,
  LESS,
  LESS_EQUAL,
  GREATER,
  GREATER_EQUAL,
  AMP_AMP,
  PIPE_PIPE,
  COMMA
};

const char*
id2str(Id id);

struct G
{
  Id mId{ YYEOF };              // 词号
  std::string_view mText;       // 对应文本
  std::string mFile;            // 文件路径
  int mLine{ 1 }, mColumn{ 1 }; // 行号、扫描列号（下一个字符位置）
  int mTokenColumn{ 1 };        // 当前 token 起始列号
  int mLastTokenLine{ 1 };      // 最近一个非 EOF token 的行号
  int mLastTokenEndColumn{ 1 }; // 最近一个非 EOF token 的结束列号（后一列）
  bool mStartOfLine{ true };    // 是否是行首
  bool mLeadingSpace{ false };  // 是否有前导空格
};

extern G g;

int
come(int tokenId, const char* yytext, int yyleng, int yylineno);

void
handleLineDirective(const char* yytext, int yyleng);

} // namespace lex
