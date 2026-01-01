#include "optimizer.h"
#include <iostream>
#include <limits>

Optimizer::Optimizer(bool enable_opt) 
    : optimization_enabled(enable_opt), optimizations_applied(0) {}

void Optimizer::optimize(CompUnit& root) {
    if (!optimization_enabled) {
        return;
    }
    
    optimizations_applied = 0;
    root.accept(*this);
    
    #ifdef DEBUG_OPTIMIZATION
    std::cerr << "Applied " << optimizations_applied << " optimizations" << std::endl;
    #endif
}

int Optimizer::get_optimizations_count() const {
    return optimizations_applied;
}

void Optimizer::visit(CompUnit& node) {
    for (const auto& func : node.functions) {
        func->accept(*this);
    }
}

void Optimizer::visit(FuncDef& node) {
    node.body->accept(*this);
}

void Optimizer::visit(Block& node) {
    // 优化每个语句
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    
    // 死代码消除
    eliminate_dead_code(node);
}

void Optimizer::visit(ExprStmt& node) {
    if (node.expression) {
        node.expression = simplify_expression(std::move(node.expression));
    }
}

void Optimizer::visit(VarDecl& node) {
    node.init_expr = simplify_expression(std::move(node.init_expr));
}

void Optimizer::visit(AssignStmt& node) {
    node.value = simplify_expression(std::move(node.value));
}

void Optimizer::visit(IfStmt& node) {
    // 优化条件表达式
    node.condition = simplify_expression(std::move(node.condition));
    
    // 尝试优化整个if语句
    auto optimized_if = optimize_if_statement(node);
    if (optimized_if) {
        // 这里需要额外的机制来替换节点，简化实现中跳过
        record_optimization("if-statement simplification");
    }
    
    // 递归优化分支
    node.then_stmt->accept(*this);
    if (node.else_stmt) {
        node.else_stmt->accept(*this);
    }
}

void Optimizer::visit(WhileStmt& node) {
    // 优化条件表达式
    node.condition = simplify_expression(std::move(node.condition));
    
    // 尝试优化整个while语句
    auto optimized_while = optimize_while_statement(node);
    if (optimized_while) {
        record_optimization("while-statement simplification");
    }
    
    // 递归优化循环体
    node.body->accept(*this);
}

void Optimizer::visit(BreakStmt& node) {
    // break语句不需要优化
}

void Optimizer::visit(ContinueStmt& node) {
    // continue语句不需要优化
}

void Optimizer::visit(ReturnStmt& node) {
    if (node.value) {
        node.value = simplify_expression(std::move(node.value));
    }
}

void Optimizer::visit(BinaryExpr& node) {
    // 递归优化子表达式
    node.left->accept(*this);
    node.right->accept(*this);
    
    // 尝试常量折叠
    auto folded = constant_folding(node);
    if (folded) {
        // 这里需要额外的机制来替换节点，简化实现中跳过
        record_optimization("constant folding");
    }
}

void Optimizer::visit(UnaryExpr& node) {
    // 递归优化子表达式
    node.operand->accept(*this);
    
    // 尝试常量折叠
    auto folded = constant_folding(node);
    if (folded) {
        record_optimization("constant folding");
    }
}

void Optimizer::visit(NumberExpr& node) {
    // 数字字面量不需要优化
}

void Optimizer::visit(VarExpr& node) {
    // 变量引用不需要优化
}

void Optimizer::visit(CallExpr& node) {
    // 优化函数调用的参数
    for (auto& arg : node.args) {
        arg = simplify_expression(std::move(arg));
    }
}

// 私有方法实现
std::unique_ptr<Expr> Optimizer::constant_folding(BinaryExpr& expr) {
    // 检查是否两个操作数都是常量
    if (!is_constant_expr(*expr.left) || !is_constant_expr(*expr.right)) {
        return nullptr;
    }
    
    int left_val = evaluate_constant_expr(*expr.left);
    int right_val = evaluate_constant_expr(*expr.right);
    int result;
    
    switch (expr.op) {
        case BinaryExpr::Operator::ADD:
            result = left_val + right_val;
            break;
        case BinaryExpr::Operator::SUB:
            result = left_val - right_val;
            break;
        case BinaryExpr::Operator::MUL:
            result = left_val * right_val;
            break;
        case BinaryExpr::Operator::DIV:
            if (right_val == 0) return nullptr;  // 避免除零
            result = left_val / right_val;
            break;
        case BinaryExpr::Operator::MOD:
            if (right_val == 0) return nullptr;  // 避免除零
            result = left_val % right_val;
            break;
        case BinaryExpr::Operator::LT:
            result = left_val < right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::GT:
            result = left_val > right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::LE:
            result = left_val <= right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::GE:
            result = left_val >= right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::EQ:
            result = left_val == right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::NE:
            result = left_val != right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::AND:
            result = (left_val && right_val) ? 1 : 0;
            break;
        case BinaryExpr::Operator::OR:
            result = (left_val || right_val) ? 1 : 0;
            break;
        default:
            return nullptr;
    }
    
    return std::make_unique<NumberExpr>(result);
}

std::unique_ptr<Expr> Optimizer::constant_folding(UnaryExpr& expr) {
    if (!is_constant_expr(*expr.operand)) {
        return nullptr;
    }
    
    int operand_val = evaluate_constant_expr(*expr.operand);
    int result;
    
    switch (expr.op) {
        case UnaryExpr::Operator::PLUS:
            result = operand_val;
            break;
        case UnaryExpr::Operator::MINUS:
            result = -operand_val;
            break;
        case UnaryExpr::Operator::NOT:
            result = operand_val ? 0 : 1;
            break;
        default:
            return nullptr;
    }
    
    return std::make_unique<NumberExpr>(result);
}

std::unique_ptr<Expr> Optimizer::simplify_expression(std::unique_ptr<Expr> expr) {
    if (!expr) return expr;
    
    // 尝试常量折叠
    if (auto* bin_expr = dynamic_cast<BinaryExpr*>(expr.get())) {
        auto folded = constant_folding(*bin_expr);
        if (folded) {
            record_optimization("binary expression constant folding");
            return folded;
        }
        
        // 其他简化规则
        // x + 0 = x
        if (bin_expr->op == BinaryExpr::Operator::ADD && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 0) {
                record_optimization("x + 0 = x");
                return std::move(bin_expr->left);
            }
        }
        
        // 0 + x = x
        if (bin_expr->op == BinaryExpr::Operator::ADD && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 0) {
                record_optimization("0 + x = x");
                return std::move(bin_expr->right);
            }
        }
        
        // x * 1 = x
        if (bin_expr->op == BinaryExpr::Operator::MUL && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 1) {
                record_optimization("x * 1 = x");
                return std::move(bin_expr->left);
            }
        }
        
        // 1 * x = x
        if (bin_expr->op == BinaryExpr::Operator::MUL && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 1) {
                record_optimization("1 * x = x");
                return std::move(bin_expr->right);
            }
        }
        
        // x * 0 = 0
        if (bin_expr->op == BinaryExpr::Operator::MUL) {
            if ((is_constant_expr(*bin_expr->left) && evaluate_constant_expr(*bin_expr->left) == 0) ||
                (is_constant_expr(*bin_expr->right) && evaluate_constant_expr(*bin_expr->right) == 0)) {
                record_optimization("x * 0 = 0");
                return std::make_unique<NumberExpr>(0);
            }
        }
        
        // x - 0 = x
        if (bin_expr->op == BinaryExpr::Operator::SUB && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 0) {
                record_optimization("x - 0 = x");
                return std::move(bin_expr->left);
            }
        }
    }
    
    if (auto* unary_expr = dynamic_cast<UnaryExpr*>(expr.get())) {
        auto folded = constant_folding(*unary_expr);
        if (folded) {
            record_optimization("unary expression constant folding");
            return folded;
        }
        
        // --x = x
        if (unary_expr->op == UnaryExpr::Operator::MINUS) {
            if (auto* inner_unary = dynamic_cast<UnaryExpr*>(unary_expr->operand.get())) {
                if (inner_unary->op == UnaryExpr::Operator::MINUS) {
                    record_optimization("--x = x");
                    return std::move(inner_unary->operand);
                }
            }
        }
        
        // !!x = x (转换为bool值)
        if (unary_expr->op == UnaryExpr::Operator::NOT) {
            if (auto* inner_unary = dynamic_cast<UnaryExpr*>(unary_expr->operand.get())) {
                if (inner_unary->op == UnaryExpr::Operator::NOT) {
                    record_optimization("!!x simplification");
                    // 在ToyC中，这相当于 x != 0 的bool值
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::NE,
                        std::move(inner_unary->operand),
                        std::make_unique<NumberExpr>(0)
                    );
                }
            }
        }
    }
    
    return expr;
}

void Optimizer::eliminate_dead_code(Block& block) {
    auto& statements = block.statements;
    
    for (auto it = statements.begin(); it != statements.end(); ) {
        // 检查是否为死代码（在return之后的代码）
        if (it != statements.begin()) {
            auto prev_it = it - 1;
            if (is_unreachable_after(**prev_it)) {
                record_optimization("dead code elimination");
                it = statements.erase(it);
                continue;
            }
        }
        
        // 检查表达式语句是否有副作用
        if (auto* expr_stmt = dynamic_cast<ExprStmt*>(it->get())) {
            if (expr_stmt->expression && !has_side_effects(*expr_stmt->expression)) {
                record_optimization("dead expression elimination");
                it = statements.erase(it);
                continue;
            }
        }
        
        ++it;
    }
}

bool Optimizer::is_unreachable_after(Stmt& stmt) {
    return dynamic_cast<ReturnStmt*>(&stmt) != nullptr ||
           dynamic_cast<BreakStmt*>(&stmt) != nullptr ||
           dynamic_cast<ContinueStmt*>(&stmt) != nullptr;
}

std::unique_ptr<Stmt> Optimizer::optimize_if_statement(IfStmt& if_stmt) {
    // 如果条件是常量，可以简化if语句
    if (is_constant_expr(*if_stmt.condition)) {
        int cond_val = evaluate_constant_expr(*if_stmt.condition);
        if (cond_val) {
            // 条件总是真，返回then分支
            return std::move(if_stmt.then_stmt);
        } else {
            // 条件总是假，返回else分支（如果存在）
            if (if_stmt.else_stmt) {
                return std::move(if_stmt.else_stmt);
            } else {
                // 返回空语句
                return std::make_unique<ExprStmt>(nullptr);
            }
        }
    }
    
    return nullptr;  // 无法优化
}

std::unique_ptr<Stmt> Optimizer::optimize_while_statement(WhileStmt& while_stmt) {
    // 如果条件是常量0，整个循环可以消除
    if (is_constant_expr(*while_stmt.condition)) {
        int cond_val = evaluate_constant_expr(*while_stmt.condition);
        if (cond_val == 0) {
            // 循环永远不执行
            return std::make_unique<ExprStmt>(nullptr);
        }
        // 如果条件是非零常量，保持原样（无限循环）
    }
    
    return nullptr;  // 无法优化
}

bool Optimizer::is_constant_expr(const Expr& expr) const {
    if (dynamic_cast<const NumberExpr*>(&expr)) {
        return true;
    }
    
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return is_constant_expr(*bin_expr->left) && is_constant_expr(*bin_expr->right);
    }
    
    if (auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return is_constant_expr(*unary_expr->operand);
    }
    
    return false;
}

int Optimizer::evaluate_constant_expr(const Expr& expr) const {
    if (auto* num_expr = dynamic_cast<const NumberExpr*>(&expr)) {
        return num_expr->value;
    }
    
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        int left_val = evaluate_constant_expr(*bin_expr->left);
        int right_val = evaluate_constant_expr(*bin_expr->right);
        
        switch (bin_expr->op) {
            case BinaryExpr::Operator::ADD: return left_val + right_val;
            case BinaryExpr::Operator::SUB: return left_val - right_val;
            case BinaryExpr::Operator::MUL: return left_val * right_val;
            case BinaryExpr::Operator::DIV: return right_val != 0 ? left_val / right_val : 0;
            case BinaryExpr::Operator::MOD: return right_val != 0 ? left_val % right_val : 0;
            case BinaryExpr::Operator::LT: return left_val < right_val ? 1 : 0;
            case BinaryExpr::Operator::GT: return left_val > right_val ? 1 : 0;
            case BinaryExpr::Operator::LE: return left_val <= right_val ? 1 : 0;
            case BinaryExpr::Operator::GE: return left_val >= right_val ? 1 : 0;
            case BinaryExpr::Operator::EQ: return left_val == right_val ? 1 : 0;
            case BinaryExpr::Operator::NE: return left_val != right_val ? 1 : 0;
            case BinaryExpr::Operator::AND: return (left_val && right_val) ? 1 : 0;
            case BinaryExpr::Operator::OR: return (left_val || right_val) ? 1 : 0;
        }
    }
    
    if (auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        int operand_val = evaluate_constant_expr(*unary_expr->operand);
        
        switch (unary_expr->op) {
            case UnaryExpr::Operator::PLUS: return operand_val;
            case UnaryExpr::Operator::MINUS: return -operand_val;
            case UnaryExpr::Operator::NOT: return operand_val ? 0 : 1;
        }
    }
    
    return 0;  // 默认值
}

bool Optimizer::has_side_effects(const Expr& expr) const {
    // 函数调用有副作用
    if (dynamic_cast<const CallExpr*>(&expr)) {
        return true;
    }
    
    // 其他表达式类型在ToyC中没有副作用
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return has_side_effects(*bin_expr->left) || has_side_effects(*bin_expr->right);
    }
    
    if (auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return has_side_effects(*unary_expr->operand);
    }
    
    return false;  // 变量引用、数字字面量等没有副作用
}

void Optimizer::record_optimization(const std::string& type) {
    optimizations_applied++;
    
    #ifdef DEBUG_OPTIMIZATION
    std::cerr << "Applied optimization: " << type << std::endl;
    #endif
}
