#pragma once
#include "ir.h"
#include "parser/ast.h"
#include "semantic/semantic.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include <functional>
#include <queue>
#include <unordered_set>

using BlockID = int;

// ==================== 异常和配置 ====================

class IRGenError : public std::runtime_error {
public:
    IRGenError(const std::string& message) : std::runtime_error(message) {}
};

struct IRGenConfig {
    bool enableOptimizations = false;
    bool generateDebugInfo = false;
    bool inlineSmallFunctions = false;
};

// ==================== IR优化器接口 ====================

class IROptimizer {
public:
    virtual ~IROptimizer() = default;
    virtual void optimize(std::vector<std::shared_ptr<IRInstr>>& instructions) = 0;
};

class ConstantFoldingOptimizer : public IROptimizer {
public:
    void optimize(std::vector<std::shared_ptr<IRInstr>>& instructions) override;
private:
    bool evaluateConstantExpression(OpCode op, int left, int right, int& result);
};

class DeadCodeOptimizer : public IROptimizer {
public:
    void optimize(std::vector<std::shared_ptr<IRInstr>>& instructions) override;
private:
    std::vector<bool> findLiveInstructions(const std::vector<std::shared_ptr<IRInstr>>& instructions);
    bool isInstructionLive(const std::shared_ptr<IRInstr>& instr, 
                          const std::map<std::string, bool>& liveVars);
};

class IRToRISCVGenerator {
public:
    virtual ~IRToRISCVGenerator() = default;
    
    virtual void generate(const std::vector<std::shared_ptr<IRInstr>>& instructions, 
                         const std::string& outputFile) = 0;
                         
    virtual std::vector<std::string> translateInstruction(const std::shared_ptr<IRInstr>& instr) = 0;
};

// ==================== IR生成器主类 ====================

class IRGenerator : public ASTVisitor {
private:
    std::vector<std::shared_ptr<IRInstr>> instructions;
    std::map<std::string, std::shared_ptr<Operand>> variables;
    std::vector<std::shared_ptr<Operand>> operandStack;
    std::vector<std::map<std::string, std::shared_ptr<Operand>>> scopeStack;
    
    int tempCount = 0;
    int labelCount = 0;
    int scopeDepth = 0;
    
    std::string currentFunction;
    std::string currentFunctionReturnType;
    
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
    std::set<std::string> usedFunctions;
    
    IRGenConfig config;

public:
    struct BasicBlock {
        int id;
        std::vector<std::shared_ptr<IRInstr>> instructions;
        std::vector<std::shared_ptr<BasicBlock>> successors;
        std::vector<std::shared_ptr<BasicBlock>> predecessors;
        std::string label;
        std::string functionName;
    };

    struct Expression {
        OpCode op;
        std::string lhs;
        std::string rhs;
        bool someFlag;

        bool operator==(const Expression& other) const {
            return op == other.op &&
                lhs == other.lhs &&
                rhs == other.rhs &&
                someFlag == other.someFlag;
        }
    };
    
    struct ExpressionHash {
        std::size_t operator()(const Expression& e) const {
            std::string op;
            switch (e.op) {
                case OpCode::ADD: op = "ADD";
                case OpCode::SUB: op = "SUB";
                case OpCode::MUL: op = "MUL";
                case OpCode::DIV: op = "DIV";
                case OpCode::MOD: op = "MOD";
                case OpCode::NEG: op = "NEG";
                case OpCode::NOT: op = "NOT";
                case OpCode::LT:  op = "LT";
                case OpCode::GT:  op = "GT";
                case OpCode::LE:  op = "LE";
                case OpCode::GE:  op = "GE";
                case OpCode::EQ:  op = "EQ";
                case OpCode::NE:  op = "NE";
                case OpCode::AND: op = "AND";
                case OpCode::OR:  op = "OR";
                case OpCode::ASSIGN: op = "ASSIGN";
                case OpCode::GOTO: op = "GOTO";
                case OpCode::IF_GOTO: op = "IF_GOTO";
                case OpCode::PARAM: op = "PARAM";
                case OpCode::CALL: op = "CALL";
                case OpCode::RETURN: op = "RETURN";
                case OpCode::LABEL: op = "LABEL";
                case OpCode::FUNCTION_BEGIN: op = "FUNCTION_BEGIN";
                case OpCode::FUNCTION_END: op = "FUNCTION_END";
                default: op = "UNKNOWN_OP";
            }
            return std::hash<std::string>()(op + "|" + e.lhs + "|" + e.rhs);
        }
    };

public:
    IRGenerator(const IRGenConfig& config = IRGenConfig()) : config(config) {
        enterScope();
    }
    
    const std::vector<std::shared_ptr<IRInstr>>& getInstructions() const { 
        return instructions; 
    }
    
    void generate(std::shared_ptr<CompUnit> ast);
    void dumpIR(const std::string& filename) const;
    void optimize();

    std::shared_ptr<Operand> createTemp();
    std::shared_ptr<Operand> createLabel();
    void addInstruction(std::shared_ptr<IRInstr> instr);
    std::shared_ptr<Operand> getTopOperand();

    const std::set<std::string>& getUsedFunctions() const {
        return usedFunctions;
    }
    
    void visit(NumberExpr& expr) override;
    void visit(VariableExpr& expr) override;
    void visit(BinaryExpr& expr) override;
    void visit(UnaryExpr& expr) override;
    void visit(CallExpr& expr) override;
    
    void visit(ExprStmt& stmt) override;
    void visit(VarDeclStmt& stmt) override;
    void visit(AssignStmt& stmt) override;
    void visit(BlockStmt& stmt) override;
    void visit(IfStmt& stmt) override;
    void visit(WhileStmt& stmt) override;
    void visit(BreakStmt& stmt) override;
    void visit(ContinueStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    
    void visit(FunctionDef& funcDef) override;
    void visit(CompUnit& compUnit) override;
    
private:
    std::shared_ptr<Operand> getVariable(const std::string& name, bool createInCurrentScope = false);

    std::string getScopedVariableName(const std::string& name) {
        return name + "_scope" + std::to_string(scopeDepth);
    }

    void enterScope();
    void exitScope();
    
    std::shared_ptr<Operand> findVariableInCurrentScope(const std::string& name);
    std::shared_ptr<Operand> findVariable(const std::string& name);
    void defineVariable(const std::string& name, std::shared_ptr<Operand> var);
    
    void constantFolding();
    void constantPropagationCFG();
    void deadCodeElimination();
    void copyPropagationCFG();
    void controlFlowOptimization();
    void commonSubexpressionElimination();
    void loopInvariantCodeMotion();
    void functionInlining();

    bool isLoopInvariant(const std::shared_ptr<IRInstr>& instr,
                         const std::unordered_set<std::string>& loopDefs);

    bool shouldInlineFunction(const std::string& funcName,
                              const std::vector<std::shared_ptr<IRInstr>>& body);
    std::shared_ptr<IRInstr> cloneInstruction(
        const std::shared_ptr<IRInstr>& instr,
        const std::unordered_map<std::string, std::string>& paramMap,
        std::unordered_map<std::string, std::string>& varRenames);
    std::shared_ptr<Operand> renameOperand(
        const std::shared_ptr<Operand>& operand,
        const std::unordered_map<std::string, std::string>& paramMap,
        std::unordered_map<std::string, std::string>& varRenames);

    bool isSideEffectInstr(const std::shared_ptr<IRInstr>& instr);

    std::shared_ptr<Operand> resolveConstant(
        const std::string& name,
        std::unordered_map<std::string, std::shared_ptr<Operand>>& constants,
        std::unordered_set<std::string>& visited,
        int depth = 0);
    
    std::shared_ptr<Operand> generateShortCircuitAnd(BinaryExpr& expr);
    std::shared_ptr<Operand> generateShortCircuitOr(BinaryExpr& expr);
    
    std::shared_ptr<Operand> makeConstantOperand(int v, std::string name);

    std::vector<std::shared_ptr<BasicBlock>> buildBasicBlocks();
    std::vector<std::shared_ptr<BasicBlock>> buildBasicBlocksByLabel();

    std::unordered_set<std::string> getLoopDefs(
        const std::unordered_set<BlockID>& loopBlocks,
        const std::unordered_map<BlockID, BasicBlock>& blocks);

    std::unordered_set<int> getLoopBlocks(
        const std::unordered_map<int, std::vector<int>>& cfg,
        int fromBlk, int toBlk);
   
    void buildCFG(std::vector<std::shared_ptr<BasicBlock>>& blocks);

    void updateJumpTargets(
        std::vector<std::shared_ptr<BasicBlock>>& blocks,
        const std::string& fromLabel,
        const std::string& toLabel);

    bool validateCFG(const std::vector<std::shared_ptr<BasicBlock>>& blocks);
    
    bool allPathsReturn(const std::shared_ptr<Stmt>& stmt);
    void markFunctionAsUsed(const std::string& funcName);
};