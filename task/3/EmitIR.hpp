#include "asg.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <unordered_map>
#include <vector>

class EmitIR
{
public:
  Obj::Mgr& mMgr;
  llvm::Module mMod;

  EmitIR(Obj::Mgr& mgr, llvm::LLVMContext& ctx, llvm::StringRef mid = "-");

  llvm::Module& operator()(asg::TranslationUnit* tu);

private:
  llvm::LLVMContext& mCtx;

  llvm::Type* mIntTy;
  llvm::FunctionType* mCtorTy;

  llvm::Function* mCurFunc{ nullptr };
  std::unique_ptr<llvm::IRBuilder<>> mCurIrb;
  std::unordered_map<asg::Stmt*, std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>
    mLoopTargets;

  llvm::Type* pointee_type(const asg::Type* type);
  llvm::Value* as_bool(llvm::Value* value);
  std::int64_t eval_const_int(asg::Expr* expr);
  llvm::Constant* const_init(asg::Expr* expr, const asg::Type* type);
  void init_local(llvm::Value* addr, const asg::Type* type, asg::Expr* init);
  void init_local_list(llvm::Value* addr,
                       const asg::Type* type,
                       asg::InitListExpr* init);
  void flatten_init(asg::Expr* init, std::vector<asg::Expr*>& out);
  void init_local_flat(llvm::Value* addr,
                       const asg::Type* type,
                       const std::vector<asg::Expr*>& flat,
                       std::size_t& pos);

  //============================================================================
  // 类型
  //============================================================================

  llvm::Type* operator()(const asg::Type* type);

  //============================================================================
  // 表达式
  //============================================================================

  llvm::Value* operator()(asg::Expr* obj);

  llvm::Constant* operator()(asg::IntegerLiteral* obj);
  llvm::Value* operator()(asg::DeclRefExpr* obj);
  llvm::Value* operator()(asg::ParenExpr* obj);
  llvm::Value* operator()(asg::UnaryExpr* obj);
  llvm::Value* operator()(asg::BinaryExpr* obj);
  llvm::Value* operator()(asg::CallExpr* obj);
  llvm::Value* operator()(asg::ImplicitCastExpr* obj);

  //============================================================================
  // 语句
  //============================================================================

  void operator()(asg::Stmt* obj);

  void operator()(asg::CompoundStmt* obj);
  void operator()(asg::DeclStmt* obj);
  void operator()(asg::ExprStmt* obj);
  void operator()(asg::NullStmt* obj);
  void operator()(asg::IfStmt* obj);
  void operator()(asg::WhileStmt* obj);
  void operator()(asg::BreakStmt* obj);
  void operator()(asg::ContinueStmt* obj);
  void operator()(asg::ReturnStmt* obj);

  //============================================================================
  // 声明
  //============================================================================

  void operator()(asg::Decl* obj);

  void operator()(asg::VarDecl* obj);
  void operator()(asg::FunctionDecl* obj);
};
