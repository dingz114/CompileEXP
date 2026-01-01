#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <cstring>

#include "ast.h"
#include "semantic_analyzer.h"
#include "code_generator.h"
#include "optimizer.h"

// 外部声明（由Flex/Bison生成）
extern int yyparse();
extern std::unique_ptr<CompUnit> ast_root;
extern FILE* yyin;

// 程序使用说明
void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [-opt] < input.tc > output.s\n";
    std::cerr << "Options:\n";
    std::cerr << "  -opt    Enable optimizations\n";
    std::cerr << "\nExample:\n";
    std::cerr << "  " << program_name << " < test.tc > test.s\n";
    std::cerr << "  " << program_name << " -opt < test.tc > test.s\n";
}

int main(int argc, char* argv[]) {
    bool enable_optimization = false;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-opt") == 0) {
            enable_optimization = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Error: Unknown option '" << argv[i] << "'\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    try {
        // 词法和语法分析
        yyin = stdin;
        
        if (yyparse() != 0) {
            std::cerr << "Error: Syntax analysis failed\n";
            return 1;
        }
        
        if (!ast_root) {
            std::cerr << "Error: Failed to build AST\n";
            return 1;
        }
        
        // 调试输出：打印AST（可选）
        #ifdef DEBUG_AST
        std::cerr << "=== Abstract Syntax Tree ===\n";
        ast_root->print();
        std::cerr << "============================\n";
        #endif
        
        // 语义分析
        SemanticAnalyzer semantic_analyzer;
        if (!semantic_analyzer.analyze(*ast_root)) {
            std::cerr << "Error: Semantic analysis failed\n";
            semantic_analyzer.print_errors();
            return 1;
        }
        
        #ifdef DEBUG_SEMANTIC
        std::cerr << "=== Semantic Analysis Passed ===\n";
        #endif
        
        // 优化（如果启用）
        if (enable_optimization) {
            Optimizer optimizer(true);
            optimizer.optimize(*ast_root);
            
            #ifdef DEBUG_OPTIMIZATION
            std::cerr << "=== Optimization Applied: " 
                      << optimizer.get_optimizations_count() << " optimizations ===\n";
            #endif
        }
        
        // 代码生成
        CodeGenerator code_generator(&semantic_analyzer.get_symbol_table());
        std::string assembly_code = code_generator.generate(*ast_root);
        
        // 输出汇编代码到stdout
        std::cout << assembly_code;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
    }
}
