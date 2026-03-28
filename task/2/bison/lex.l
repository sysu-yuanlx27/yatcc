%{
#include "lex.hpp"
/* 所有代码全部抽离出来，放到 lex.hpp 和 lex.cpp 里 */

using namespace lex;

#define ADDCOL() g.mColumn += yyleng;
#define COME(id) return come(id, yytext, yyleng, yylineno)
#define COME_RAW(id)               \
	do {                             \
		yylval.RawStr =                \
			new std::string(yytext, yyleng); \
		return come(id, yytext, yyleng, yylineno); \
	} while (0)
#define COME_LINE() return come_line(yytext, yyleng, yylineno)
%}

%option 8bit warn noyywrap yylineno

D     [0-9]
L     [a-zA-Z_]
H     [a-fA-F0-9]
IS    ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))


%%

^#[^\n]*\n             { g.mColumn = 1; }

^([a-z_]+)[ \t]+'[^'\n]*'[ \t]*\n   { COME_LINE(); }

"int"       { ADDCOL(); COME(INT); }
"const"     { ADDCOL(); COME(CONST); }
"void"      { ADDCOL(); COME(VOID); }
"if"        { ADDCOL(); COME(IF); }
"else"      { ADDCOL(); COME(ELSE); }
"while"     { ADDCOL(); COME(WHILE); }
"break"     { ADDCOL(); COME(BREAK); }
"continue"  { ADDCOL(); COME(CONTINUE); }
"return"    { ADDCOL(); COME(RETURN); }

"("         { ADDCOL(); COME('('); }
")"         { ADDCOL(); COME(')'); }
"["         { ADDCOL(); COME('['); }
"]"         { ADDCOL(); COME(']'); }
"{"         { ADDCOL(); COME('{'); }
"}"         { ADDCOL(); COME('}'); }

"+"         { ADDCOL(); COME('+'); }
"-"         { ADDCOL(); COME('-'); }
"*"         { ADDCOL(); COME('*'); }
"/"         { ADDCOL(); COME('/'); }
"%"         { ADDCOL(); COME('%'); }

"=="        { ADDCOL(); COME(EQ_OP); }
"="         { ADDCOL(); COME('='); }
"!="        { ADDCOL(); COME(NE_OP); }
"!"         { ADDCOL(); COME('!'); }

"<="        { ADDCOL(); COME(LE_OP); }
"<"         { ADDCOL(); COME('<'); }
">="        { ADDCOL(); COME(GE_OP); }
">"         { ADDCOL(); COME('>'); }

"&&"        { ADDCOL(); COME(AND_OP); }
"||"        { ADDCOL(); COME(OR_OP); }

";"         { ADDCOL(); COME(';'); }
","         { ADDCOL(); COME(','); }

{L}({L}|{D})*         { ADDCOL(); COME_RAW(IDENTIFIER); }

[0][xX]{H}+{IS}?      { ADDCOL(); COME_RAW(CONSTANT); }
0[0-7]*{IS}?          { ADDCOL(); COME_RAW(CONSTANT); }
[1-9]{D}*{IS}?        { ADDCOL(); COME_RAW(CONSTANT); }

[ \t\v\f\r]+        { g.mColumn += yyleng; }

\n                   { g.mColumn = 1; }

<<EOF>>               { return YYEOF; }

.                     { COME(YYUNDEF); }

%%

/* about symbols avaliable (yytext, yyleng etc.) in the context of Flex:
 * https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_14.html
 * https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_15.html
*/
