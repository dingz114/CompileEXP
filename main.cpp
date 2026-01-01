// main.cpp - 编译器主程序
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "ir/ir.h"
#include "ir/irgen.h"
#include "codegen/codegen.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    bool enableOptimization = false;
    bool enablePrintIR = true;
    
    std::string filename;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-opt") {
            enableOptimization = true;
            std::cerr << "Optimization enabled." << std::endl;
        } else {
            filename = arg;
        }
    }
    
    std::stringstream buffer;
    
    if (!filename.empty()) {
        std::ifstream inputFile(filename);
        if (!inputFile) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return 1;
        }
        buffer << inputFile.rdbuf();
    } else {
        buffer << std::cin.rdbuf();
    }
    
    std::string source = buffer.str();
    
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    
    Parser parser(tokens);
    std::shared_ptr<CompUnit> ast = parser.parse();
    if (!ast) {
        std::cerr << "Error: Parsing failed." << std::endl;
        return 1;
    }
    
    SemanticAnalyzer semanticAnalyzer;
    if (!semanticAnalyzer.analyze(ast)) {
        std::cerr << "Error: Semantic analysis failed." << std::endl;
        return 1;
    }

    IRGenConfig irConfig;
    if (enableOptimization) {
        irConfig.enableOptimizations = true;
    }
    
    IRGenerator irGenerator(irConfig);
    irGenerator.generate(ast);
    
    if (enablePrintIR) {
        IRPrinter::print(irGenerator.getInstructions(), std::cerr);
    }
    
    CodeGenConfig config;
    
    std::stringstream outputStream;
    
    CodeGenerator generator(outputStream, irGenerator.getInstructions(), config);
    generator.generate();
    
    std::cout << outputStream.str();
    
    return 0;
}