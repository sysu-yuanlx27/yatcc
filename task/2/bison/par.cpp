#include "par.hpp"
#include "lex.hpp"
#include <cstdint>
#include <fstream>
#include <stack>
#include <unordered_map>

namespace par {

Obj::Mgr gMgr;
asg::TranslationUnit* gTranslationUnit;
asg::FunctionDecl* gCurrentFunction;

Symtbl* Symtbl::g{ nullptr };

std::int64_t
eval_constexpr(asg::Expr* expr)
{
  if (auto p = expr->dcst<asg::IntegerLiteral>())
    return static_cast<std::int64_t>(p->val);

  if (auto p = expr->dcst<asg::DeclRefExpr>()) {
    if (p->decl == nullptr)
      ABORT();

    auto var = p->decl->dcst<asg::VarDecl>();
    if (var == nullptr || !var->type->qual.const_ || var->init == nullptr)
      ABORT();

    return eval_constexpr(var->init);
  }

  if (auto p = expr->dcst<asg::UnaryExpr>()) {
    auto sub = eval_constexpr(p->sub);
    switch (p->op) {
      case asg::UnaryExpr::kPos:
        return sub;
      case asg::UnaryExpr::kNeg:
        return -sub;
      default:
        ABORT();
    }
  }

  if (auto p = expr->dcst<asg::BinaryExpr>()) {
    auto lft = eval_constexpr(p->lft);
    auto rht = eval_constexpr(p->rht);
    switch (p->op) {
      case asg::BinaryExpr::kAdd:
        return lft + rht;
      case asg::BinaryExpr::kSub:
        return lft - rht;
      case asg::BinaryExpr::kMul:
        return lft * rht;
      case asg::BinaryExpr::kDiv:
        if (rht == 0)
          ABORT();
        return lft / rht;
      case asg::BinaryExpr::kMod:
        if (rht == 0)
          ABORT();
        return lft % rht;
      default:
        ABORT();
    }
  }

  if (auto p = expr->dcst<asg::InitListExpr>()) {
    if (p->list.empty())
      return 0;
    return eval_constexpr(p->list.front());
  }

  ABORT();
}

asg::Decl*
Symtbl::resolve(const std::string& name)
{
  auto cur = g;
  while (cur) {
    auto iter = cur->find(name);
    if (iter != cur->end())
      return iter->second;
    cur = cur->mPrev;
  }
  return nullptr; // 标识符未定义
}

} // namespace par

void
yyerror(char const* s)
{
  fflush(stdout);
  printf("\n%*s\n%*s\n", lex::g.mLine, "^", lex::g.mColumn, s);
}
