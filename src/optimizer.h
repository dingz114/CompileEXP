#pragma once

#include "ast.h"
#include <memory>

// 优化器类
class Optimizer : public ASTVisitor {
private:
    bool optimization_enabled;
    int optimizations_applied;
    
public:
    explicit Optimizer(bool enable_opt = false);
    
    // 优化入口
    void optimize(CompUnit& root);
    
    // 获取优化统计信息
    int get_optimizations_count() const;
    
    // AST访问方法
    void visit(CompUnit& node) override;
    void visit(FuncDef& node) override;
    void visit(Block& node) override;
    void visit(ExprStmt& node) override;
    void visit(VarDecl& node) override;
    void visit(AssignStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(BreakStmt& node) override;
    void visit(ContinueStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(NumberExpr& node) override;
    void visit(VarExpr& node) override;
    void visit(CallExpr& node) override;
    
private:
    // 常量折叠优化
    std::unique_ptr<Expr> constant_folding(BinaryExpr& expr);
    std::unique_ptr<Expr> constant_folding(UnaryExpr& expr);
    
    // 表达式简化
    std::unique_ptr<Expr> simplify_expression(std::unique_ptr<Expr> expr);
    
    // 死代码消除
    void eliminate_dead_code(Block& block);
    bool is_unreachable_after(Stmt& stmt);
    
    // 控制流优化
    std::unique_ptr<Stmt> optimize_if_statement(IfStmt& if_stmt);
    std::unique_ptr<Stmt> optimize_while_statement(WhileStmt& while_stmt);
    
    // 工具函数
    bool is_constant_expr(const Expr& expr) const;
    int evaluate_constant_expr(const Expr& expr) const;
    bool has_side_effects(const Expr& expr) const;
    
    // 优化统计
    void record_optimization(const std::string& type);
};
