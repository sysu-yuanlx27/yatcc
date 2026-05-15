#include "EmitIR.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>

#define self (*this)

using namespace asg;

EmitIR::EmitIR(Obj::Mgr& mgr, llvm::LLVMContext& ctx, llvm::StringRef mid)
  : mMgr(mgr)
  , mMod(mid, ctx)
  , mCtx(ctx)
  , mIntTy(llvm::Type::getInt32Ty(ctx))
  , mCtorTy(llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false))
  , mCurIrb(std::make_unique<llvm::IRBuilder<>>(ctx))
{
}

llvm::Module&
EmitIR::operator()(TranslationUnit* tu)
{
  for (auto&& i : tu->decls)
    self(i);
  return mMod;
}

llvm::Type*
EmitIR::pointee_type(const Type* type)
{
  ASSERT(type->texp);
  Type subt;
  subt.spec = type->spec;
  subt.qual = type->qual;
  subt.texp = type->texp->sub;
  return self(&subt);
}

llvm::Value*
EmitIR::as_bool(llvm::Value* value)
{
  if (value->getType()->isIntegerTy(1))
    return value;
  return mCurIrb->CreateICmpNE(
    value, llvm::ConstantInt::get(value->getType(), 0), "tobool");
}

llvm::Constant*
EmitIR::const_init(Expr* expr, const Type* type)
{
  if (!expr)
    return llvm::Constant::getNullValue(self(type));
  if (expr->dcst<ImplicitInitExpr>())
    return llvm::Constant::getNullValue(self(type));
  if (auto p = expr->dcst<InitListExpr>()) {
    auto arr = type->texp->dcst<ArrayType>();
    ASSERT(arr);
    Type elem;
    elem.spec = type->spec;
    elem.qual = type->qual;
    elem.texp = type->texp->sub;
    std::vector<llvm::Constant*> elems;
    elems.reserve(arr->len);
    for (std::uint32_t i = 0; i < arr->len; ++i) {
      Expr* child = i < p->list.size() ? p->list[i] : nullptr;
      elems.push_back(const_init(child, &elem));
    }
    return llvm::ConstantArray::get(
      llvm::cast<llvm::ArrayType>(self(type)), elems);
  }
  return llvm::ConstantInt::get(self(type), eval_const_int(expr), true);
}

std::int64_t
EmitIR::eval_const_int(Expr* expr)
{
  if (auto p = expr->dcst<IntegerLiteral>())
    return p->val;
  if (auto p = expr->dcst<ParenExpr>())
    return eval_const_int(p->sub);
  if (auto p = expr->dcst<ImplicitCastExpr>())
    return eval_const_int(p->sub);
  if (auto p = expr->dcst<UnaryExpr>()) {
    auto v = eval_const_int(p->sub);
    switch (p->op) {
      case UnaryExpr::kPos:
        return v;
      case UnaryExpr::kNeg:
        return -v;
      case UnaryExpr::kNot:
        return !v;
      default:
        ABORT();
    }
  }
  if (auto p = expr->dcst<BinaryExpr>()) {
    auto lhs = eval_const_int(p->lft);
    auto rhs = eval_const_int(p->rht);
    switch (p->op) {
      case BinaryExpr::kMul:
        return lhs * rhs;
      case BinaryExpr::kDiv:
        return lhs / rhs;
      case BinaryExpr::kMod:
        return lhs % rhs;
      case BinaryExpr::kAdd:
        return lhs + rhs;
      case BinaryExpr::kSub:
        return lhs - rhs;
      case BinaryExpr::kGt:
        return lhs > rhs;
      case BinaryExpr::kLt:
        return lhs < rhs;
      case BinaryExpr::kGe:
        return lhs >= rhs;
      case BinaryExpr::kLe:
        return lhs <= rhs;
      case BinaryExpr::kEq:
        return lhs == rhs;
      case BinaryExpr::kNe:
        return lhs != rhs;
      case BinaryExpr::kAnd:
        return lhs && rhs;
      case BinaryExpr::kOr:
        return lhs || rhs;
      default:
        ABORT();
    }
  }
  ABORT();
}

void
EmitIR::init_local(llvm::Value* addr, const Type* type, Expr* init)
{
  if (!init) {
    mCurIrb->CreateStore(llvm::Constant::getNullValue(self(type)), addr);
    return;
  }
  if (auto p = init->dcst<InitListExpr>()) {
    init_local_list(addr, type, p);
    return;
  }
  if (init->dcst<ImplicitInitExpr>()) {
    mCurIrb->CreateStore(llvm::Constant::getNullValue(self(type)), addr);
    return;
  }
  mCurIrb->CreateStore(self(init), addr);
}

void
EmitIR::init_local_list(llvm::Value* addr, const Type* type, InitListExpr* init)
{
  std::vector<Expr*> flat;
  flatten_init(init, flat);
  std::size_t pos = 0;
  init_local_flat(addr, type, flat, pos);
}

void
EmitIR::flatten_init(Expr* init, std::vector<Expr*>& out)
{
  if (auto list = init->dcst<InitListExpr>()) {
    for (auto&& child : list->list)
      flatten_init(child, out);
    return;
  }
  out.push_back(init);
}

void
EmitIR::init_local_flat(llvm::Value* addr,
                        const Type* type,
                        const std::vector<Expr*>& flat,
                        std::size_t& pos)
{
  auto arr = type->texp->dcst<ArrayType>();
  if (!arr) {
    Expr* init = pos < flat.size() ? flat[pos++] : nullptr;
    init_local(addr, type, init);
    return;
  }

  Type elem;
  elem.spec = type->spec;
  elem.qual = type->qual;
  elem.texp = type->texp->sub;
  auto zero = llvm::ConstantInt::get(mIntTy, 0);
  for (std::uint32_t i = 0; i < arr->len; ++i) {
    auto idx = llvm::ConstantInt::get(mIntTy, i);
    auto elemAddr =
      mCurIrb->CreateInBoundsGEP(self(type), addr, { zero, idx }, "elemptr");
    init_local_flat(elemAddr, &elem, flat, pos);
  }
}

//==============================================================================
// 类型
//==============================================================================

llvm::Type*
EmitIR::operator()(const Type* type)
{
  if (type->texp == nullptr) {
    switch (type->spec) {
      case Type::Spec::kVoid:
        return llvm::Type::getVoidTy(mCtx);
      case Type::Spec::kChar:
        return llvm::Type::getInt8Ty(mCtx);
      case Type::Spec::kInt:
        return llvm::Type::getInt32Ty(mCtx);
      case Type::Spec::kLong:
      case Type::Spec::kLongLong:
        return llvm::Type::getInt64Ty(mCtx);
      default:
        ABORT();
    }
  }

  Type subt;
  subt.spec = type->spec;
  subt.qual = type->qual;
  subt.texp = type->texp->sub;

  if (type->texp->dcst<PointerType>())
    return llvm::PointerType::get(mCtx, 0);

  if (auto p = type->texp->dcst<ArrayType>())
    return llvm::ArrayType::get(self(&subt), p->len);

  if (auto p = type->texp->dcst<FunctionType>()) {
    std::vector<llvm::Type*> pty;
    for (auto&& param : p->params)
      pty.push_back(self(param));
    return llvm::FunctionType::get(self(&subt), std::move(pty), false);
  }

  ABORT();
}

//==============================================================================
// 表达式
//==============================================================================

llvm::Value*
EmitIR::operator()(Expr* obj)
{
  if (auto p = obj->dcst<IntegerLiteral>())
    return self(p);
  if (auto p = obj->dcst<DeclRefExpr>())
    return self(p);
  if (auto p = obj->dcst<ParenExpr>())
    return self(p);
  if (auto p = obj->dcst<UnaryExpr>())
    return self(p);
  if (auto p = obj->dcst<BinaryExpr>())
    return self(p);
  if (auto p = obj->dcst<CallExpr>())
    return self(p);
  if (auto p = obj->dcst<ImplicitCastExpr>())
    return self(p);
  ABORT();
}

llvm::Constant*
EmitIR::operator()(IntegerLiteral* obj)
{
  return llvm::ConstantInt::get(self(obj->type), obj->val);
}

llvm::Value*
EmitIR::operator()(DeclRefExpr* obj)
{
  return obj->decl->any_as<llvm::Value>();
}

llvm::Value*
EmitIR::operator()(ParenExpr* obj)
{
  return self(obj->sub);
}

llvm::Value*
EmitIR::operator()(UnaryExpr* obj)
{
  auto v = self(obj->sub);
  switch (obj->op) {
    case UnaryExpr::kPos:
      return v;
    case UnaryExpr::kNeg:
      return mCurIrb->CreateNeg(v, "neg");
    case UnaryExpr::kNot: {
      auto b = mCurIrb->CreateNot(as_bool(v), "not");
      return mCurIrb->CreateZExt(b, self(obj->type), "not.ext");
    }
    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(BinaryExpr* obj)
{
  auto& irb = *mCurIrb;
  if (obj->op == BinaryExpr::kAssign) {
    auto addr = self(obj->lft);
    auto val = self(obj->rht);
    irb.CreateStore(val, addr);
    return val;
  }
  if (obj->op == BinaryExpr::kIndex) {
    auto base = self(obj->lft);
    auto idx = self(obj->rht);
    return irb.CreateInBoundsGEP(pointee_type(obj->lft->type),
                                 base,
                                 { idx },
                                 "idxptr");
  }
  if (obj->op == BinaryExpr::kAnd || obj->op == BinaryExpr::kOr) {
    auto lhsVal = self(obj->lft);
    auto lhsBool = as_bool(lhsVal);
    auto lhsBb = irb.GetInsertBlock();
    auto rhsBb = llvm::BasicBlock::Create(mCtx, "logic.rhs", mCurFunc);
    auto endBb = llvm::BasicBlock::Create(mCtx, "logic.end", mCurFunc);
    if (obj->op == BinaryExpr::kAnd)
      irb.CreateCondBr(lhsBool, rhsBb, endBb);
    else
      irb.CreateCondBr(lhsBool, endBb, rhsBb);

    irb.SetInsertPoint(rhsBb);
    auto rhsBool = as_bool(self(obj->rht));
    auto rhsEndBb = irb.GetInsertBlock();
    irb.CreateBr(endBb);

    irb.SetInsertPoint(endBb);
    auto phi = irb.CreatePHI(llvm::Type::getInt1Ty(mCtx), 2, "logic");
    phi->addIncoming(
      llvm::ConstantInt::get(llvm::Type::getInt1Ty(mCtx),
                             obj->op == BinaryExpr::kOr),
      lhsBb);
    phi->addIncoming(rhsBool, rhsEndBb);
    return irb.CreateZExt(phi, self(obj->type), "logic.ext");
  }

  auto lhs = self(obj->lft);
  auto rhs = self(obj->rht);
  switch (obj->op) {
    case BinaryExpr::kMul:
      return irb.CreateMul(lhs, rhs, "mul");
    case BinaryExpr::kDiv:
      return irb.CreateSDiv(lhs, rhs, "div");
    case BinaryExpr::kMod:
      return irb.CreateSRem(lhs, rhs, "mod");
    case BinaryExpr::kAdd:
      return irb.CreateAdd(lhs, rhs, "add");
    case BinaryExpr::kSub:
      return irb.CreateSub(lhs, rhs, "sub");
    case BinaryExpr::kGt:
    case BinaryExpr::kLt:
    case BinaryExpr::kGe:
    case BinaryExpr::kLe:
    case BinaryExpr::kEq:
    case BinaryExpr::kNe: {
      llvm::CmpInst::Predicate pred;
      switch (obj->op) {
        case BinaryExpr::kGt:
          pred = llvm::CmpInst::ICMP_SGT;
          break;
        case BinaryExpr::kLt:
          pred = llvm::CmpInst::ICMP_SLT;
          break;
        case BinaryExpr::kGe:
          pred = llvm::CmpInst::ICMP_SGE;
          break;
        case BinaryExpr::kLe:
          pred = llvm::CmpInst::ICMP_SLE;
          break;
        case BinaryExpr::kEq:
          pred = llvm::CmpInst::ICMP_EQ;
          break;
        case BinaryExpr::kNe:
          pred = llvm::CmpInst::ICMP_NE;
          break;
        default:
          ABORT();
      }
      auto cmp = irb.CreateICmp(pred, lhs, rhs, "cmp");
      return irb.CreateZExt(cmp, self(obj->type), "cmp.ext");
    }
    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(CallExpr* obj)
{
  auto callee = self(obj->head);
  std::vector<llvm::Value*> args;
  for (auto&& arg : obj->args)
    args.push_back(self(arg));
  auto fty = llvm::cast<llvm::FunctionType>(
    self(obj->head->type)->isPointerTy()
      ? pointee_type(obj->head->type)
      : self(obj->head->type));
  return mCurIrb->CreateCall(fty, callee, std::move(args));
}

llvm::Value*
EmitIR::operator()(ImplicitCastExpr* obj)
{
  auto v = self(obj->sub);
  switch (obj->kind) {
    case ImplicitCastExpr::kLValueToRValue:
      return mCurIrb->CreateLoad(self(obj->type), v, "load");
    case ImplicitCastExpr::kArrayToPointerDecay: {
      auto zero = llvm::ConstantInt::get(mIntTy, 0);
      return mCurIrb->CreateInBoundsGEP(
        self(obj->sub->type), v, { zero, zero }, "decay");
    }
    case ImplicitCastExpr::kFunctionToPointerDecay:
      return v;
    default:
      ABORT();
  }
}

//==============================================================================
// 语句
//==============================================================================

void
EmitIR::operator()(Stmt* obj)
{
  if (auto p = obj->dcst<CompoundStmt>())
    return self(p);
  if (auto p = obj->dcst<DeclStmt>())
    return self(p);
  if (auto p = obj->dcst<ExprStmt>())
    return self(p);
  if (auto p = obj->dcst<NullStmt>())
    return self(p);
  if (auto p = obj->dcst<IfStmt>())
    return self(p);
  if (auto p = obj->dcst<WhileStmt>())
    return self(p);
  if (auto p = obj->dcst<BreakStmt>())
    return self(p);
  if (auto p = obj->dcst<ContinueStmt>())
    return self(p);
  if (auto p = obj->dcst<ReturnStmt>())
    return self(p);
  ABORT();
}

void
EmitIR::operator()(CompoundStmt* obj)
{
  for (auto&& stmt : obj->subs)
    self(stmt);
}

void
EmitIR::operator()(DeclStmt* obj)
{
  for (auto&& decl : obj->decls)
    self(decl);
}

void
EmitIR::operator()(ExprStmt* obj)
{
  self(obj->expr);
}

void
EmitIR::operator()(NullStmt*)
{
}

void
EmitIR::operator()(IfStmt* obj)
{
  auto& irb = *mCurIrb;
  auto thenBb = llvm::BasicBlock::Create(mCtx, "if.then", mCurFunc);
  auto elseBb = obj->else_ ? llvm::BasicBlock::Create(mCtx, "if.else", mCurFunc)
                           : nullptr;
  auto endBb = llvm::BasicBlock::Create(mCtx, "if.end", mCurFunc);
  irb.CreateCondBr(as_bool(self(obj->cond)), thenBb, elseBb ? elseBb : endBb);

  irb.SetInsertPoint(thenBb);
  self(obj->then);
  if (!irb.GetInsertBlock()->getTerminator())
    irb.CreateBr(endBb);

  if (elseBb) {
    irb.SetInsertPoint(elseBb);
    self(obj->else_);
    if (!irb.GetInsertBlock()->getTerminator())
      irb.CreateBr(endBb);
  }

  irb.SetInsertPoint(endBb);
}

void
EmitIR::operator()(WhileStmt* obj)
{
  auto& irb = *mCurIrb;
  auto condBb = llvm::BasicBlock::Create(mCtx, "while.cond", mCurFunc);
  auto bodyBb = llvm::BasicBlock::Create(mCtx, "while.body", mCurFunc);
  auto endBb = llvm::BasicBlock::Create(mCtx, "while.end", mCurFunc);
  irb.CreateBr(condBb);

  irb.SetInsertPoint(condBb);
  irb.CreateCondBr(as_bool(self(obj->cond)), bodyBb, endBb);

  mLoopTargets[obj] = { condBb, endBb };
  irb.SetInsertPoint(bodyBb);
  self(obj->body);
  if (!irb.GetInsertBlock()->getTerminator())
    irb.CreateBr(condBb);
  mLoopTargets.erase(obj);

  irb.SetInsertPoint(endBb);
}

void
EmitIR::operator()(BreakStmt* obj)
{
  mCurIrb->CreateBr(mLoopTargets.at(obj->loop).second);
  auto dead = llvm::BasicBlock::Create(mCtx, "after.break", mCurFunc);
  mCurIrb->SetInsertPoint(dead);
}

void
EmitIR::operator()(ContinueStmt* obj)
{
  mCurIrb->CreateBr(mLoopTargets.at(obj->loop).first);
  auto dead = llvm::BasicBlock::Create(mCtx, "after.continue", mCurFunc);
  mCurIrb->SetInsertPoint(dead);
}

void
EmitIR::operator()(ReturnStmt* obj)
{
  llvm::Value* retVal = obj->expr ? self(obj->expr) : nullptr;
  mCurIrb->CreateRet(retVal);
  auto exitBb = llvm::BasicBlock::Create(mCtx, "return.exit", mCurFunc);
  mCurIrb->SetInsertPoint(exitBb);
}

//==============================================================================
// 声明
//==============================================================================

void
EmitIR::operator()(Decl* obj)
{
  if (auto p = obj->dcst<VarDecl>())
    return self(p);
  if (auto p = obj->dcst<FunctionDecl>())
    return self(p);
  ABORT();
}

void
EmitIR::operator()(VarDecl* obj)
{
  auto ty = self(obj->type);
  if (!mCurFunc) {
    auto gv = new llvm::GlobalVariable(mMod,
                                       ty,
                                       obj->type->qual.const_,
                                       llvm::GlobalValue::ExternalLinkage,
                                       const_init(obj->init, obj->type),
                                       obj->name);
    obj->any = gv;
    return;
  }

  auto addr = mCurIrb->CreateAlloca(ty, nullptr, obj->name);
  obj->any = addr;
  if (obj->init)
    init_local(addr, obj->type, obj->init);
}

void
EmitIR::operator()(FunctionDecl* obj)
{
  auto fty = llvm::dyn_cast<llvm::FunctionType>(self(obj->type));
  auto func = llvm::Function::Create(
    fty, llvm::GlobalVariable::ExternalLinkage, obj->name, mMod);
  obj->any = func;

  if (obj->body == nullptr)
    return;

  auto entryBb = llvm::BasicBlock::Create(mCtx, "entry", func);
  mCurIrb->SetInsertPoint(entryBb);
  mCurFunc = func;

  std::size_t idx = 0;
  for (auto& arg : func->args()) {
    auto* decl = obj->params[idx++];
    auto addr = mCurIrb->CreateAlloca(arg.getType(), nullptr, decl->name);
    mCurIrb->CreateStore(&arg, addr);
    decl->any = addr;
  }

  self(obj->body);
  if (!mCurIrb->GetInsertBlock()->getTerminator()) {
    if (fty->getReturnType()->isVoidTy())
      mCurIrb->CreateRetVoid();
    else
      mCurIrb->CreateUnreachable();
  }
  mCurFunc = nullptr;
}
