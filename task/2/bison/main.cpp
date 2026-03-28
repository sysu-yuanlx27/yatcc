#include "Asg2Json.hpp"
#include "Typing.hpp"
#include "lex.l.hh"
#include "par.y.hh"
#include <fstream>
#include <iostream>

extern int yydebug;

int
main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <input> <output>\n";
    return -1;
  }

  yyin = fopen(argv[1], "r");
  if (!yyin) {
    std::cerr << "Failed to open " << argv[1] << '\n';
    return -2;
  }

  std::error_code ec;
  llvm::StringRef outPath(argv[2]);
  llvm::raw_fd_ostream outFile(outPath, ec);
  if (ec) {
    std::cout << "Error: unable to open output file: " << argv[2] << '\n';
    return -3;
  }

  std::cout << "程序 " << argv[0] << std::endl;
  std::cout << "输入 " << argv[1] << std::endl;
  std::cout << "输出 " << argv[2] << std::endl;

  yydebug = 0;
  if (auto e = yyparse())
    return e;
  par::gMgr.mRoot = par::gTranslationUnit;
  par::gMgr.gc();

  asg::Typing typing(par::gMgr);
  typing(par::gTranslationUnit);
  typing.mTypeCache.clear();
  par::gMgr.gc();

  asg::Asg2Json asg2json;
  llvm::json::Value json = asg2json(par::gTranslationUnit);
  outFile << json << '\n';

  fclose(yyin);
  return 0;
}
