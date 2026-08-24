// ─────────────────────────────────────────────────────────────────────────────
// main.cpp — Blades language driver (entry point)
//
// Handles command-line argument parsing and dispatches to the appropriate
// pipeline: REPL, file execution, or compilation.
// ─────────────────────────────────────────────────────────────────────────────

#include <fstream>
#include <sstream>
#include <iostream>
#include <span>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <memory>

#include "version.hpp"

#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/semantic_analyzer.hpp"
#include "compiler/ir_generator.hpp"
#include "backend/vm.hpp"
#include "runtime/stdlib.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations (will be implemented in later phases)
// ─────────────────────────────────────────────────────────────────────────────

namespace blades
{

/// Placeholder: starts the interactive REPL (Fase 7).
static void run_repl();

/// Prints usage information to stdout.
static void print_usage(std::string_view program_name);

/// Prints version information to stdout.
static void print_version();

// ─────────────────────────────────────────────────────────────────────────────
// run_repl — stubs for future phases
// ─────────────────────────────────────────────────────────────────────────────

// We need a persistent state for the REPL
struct ExecutionState
{
    SymbolTable globals;
    VM vm;
    std::unordered_set<std::string> imported_files;
    std::vector<std::unique_ptr<std::string>> loaded_sources;
    
    ExecutionState()
    {
        // Pre-declare stdlib in semantics
        globals.declare("print", ValueType::Any);
        globals.declare("clock", ValueType::Any);
        globals.declare("type_of", ValueType::Any);
        globals.declare("random", ValueType::Any);
        globals.declare("input", ValueType::Any);
        globals.declare("len", ValueType::Any);
        
        // Inject modules in VM
        register_stdlib(vm);
    }
};

static std::vector<std::unique_ptr<Stmt>> link_ast(std::vector<std::unique_ptr<Stmt>> stmts, ExecutionState& state)
{
    std::vector<std::unique_ptr<Stmt>> linked;
    for (auto& stmt : stmts)
    {
        if (auto* import_stmt = dynamic_cast<ImportStmt*>(stmt.get()))
        {
            std::string path(import_stmt->path.lexeme);
            if (path.length() >= 2 && path.front() == '"' && path.back() == '"')
                path = path.substr(1, path.length() - 2);

            if (state.imported_files.find(path) == state.imported_files.end())
            {
                state.imported_files.insert(path);
                
                std::ifstream file(path);
                if (!file.is_open())
                {
                    std::cerr << "Linker Error: Could not open module '" << path << "'\n";
                    throw ParseError("Module not found");
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                
                auto source_str = std::make_unique<std::string>(buffer.str());
                std::string_view source_view = *source_str;
                state.loaded_sources.push_back(std::move(source_str));
                
                Lexer lexer(source_view, path);
                Parser parser(lexer);
                auto module_stmts = parser.parse();
                
                auto linked_module = link_ast(std::move(module_stmts), state);
                
                for (auto& ms : linked_module)
                {
                    linked.push_back(std::move(ms));
                }
            }
        }
        else
        {
            linked.push_back(std::move(stmt));
        }
    }
    return linked;
}

static InterpretResult execute_source(std::string_view source, const char* name, ExecutionState& state)
{
    Lexer lexer(source, name);
    Parser parser(lexer);
    
    std::vector<std::unique_ptr<Stmt>> stmts;
    try
    {
        stmts = parser.parse();
        stmts = link_ast(std::move(stmts), state);
    }
    catch (const ParseError& e)
    {
        std::cerr << e.what() << "\n";
        return InterpretResult::CompileError;
    }
    
    try
    {
        SemanticAnalyzer semantic(state.globals);
        semantic.analyze(stmts);
    }
    catch (const SemanticError& e)
    {
        std::cerr << e.what() << "\n";
        return InterpretResult::CompileError;
    }
    
    IRGenerator generator;
    auto function = generator.generate(stmts);
    
    InterpretResult result = state.vm.interpret(function);
    while (result == InterpretResult::Yield)
    {
        result = state.vm.resume(state.vm.get_current_fiber());
    }
    
    return result;
}

static void run_repl()
{
    std::cout << "Blades REPL\n";
    std::cout << "Type 'exit' or press Ctrl+C to quit.\n";
    
    ExecutionState state;
    
    std::string line;
    while (true)
    {
        std::cout << ">> ";
        if (!std::getline(std::cin, line))
        {
            std::cout << "\n";
            break; // EOF
        }
        
        if (line == "exit" || line == "quit")
        {
            break;
        }
        
        if (line.empty()) continue;
        
        execute_source(line, "repl", state);
    }
}

static void print_version()
{
    std::cout << LANGUAGE_NAME << ' ' << VERSION_STRING << '\n';
}

static void print_usage(std::string_view program_name)
{
    std::cout
        << "Usage:\n"
        << "  " << program_name << "             Start the interactive REPL\n"
        << "  " << program_name << " <file.bl>   Run a Blades source file\n"
        << "  " << program_name << " --version   Print version information\n"
        << "  " << program_name << " --help      Show this help message\n";
}

} // namespace blades

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    using namespace blades;

    auto args = std::span<char*>(argv, static_cast<std::size_t>(argc));
    std::string_view program_name = args[0];

    // No arguments → start REPL
    if (argc == 1)
    {
        print_version();
        run_repl();
        return 0;
    }

    // Single flag
    if (argc == 2)
    {
        std::string_view arg = args[1];

        if (arg == "--version" || arg == "-v")
        {
            print_version();
            return 0;
        }

        if (arg == "--help" || arg == "-h")
        {
            print_version();
            std::cout << '\n';
            print_usage(program_name);
            return 0;
        }

        // Initialize engine and run script as Entity component
        std::ifstream file(std::string{arg});
        if (!file.is_open())
        {
            std::cerr << "Could not open file: " << arg << "\n";
            return 74; // EX_IOERR
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();

        blades::ExecutionState state;

        // Evaluate the script (no engine ECS)
        InterpretResult result = execute_source(buffer.str(), arg.data(), state);
        
        return result == InterpretResult::Ok ? 0 : 1;
    }

    // Unknown usage
    std::cerr << "error: unexpected arguments\n\n";
    print_usage(program_name);
    return 1;
}
