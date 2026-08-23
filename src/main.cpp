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

/// Placeholder: runs a .bl source file through the full pipeline (Fase 5+).
static void run_file(std::string_view path);

/// Prints usage information to stdout.
static void print_usage(std::string_view program_name);

/// Prints version information to stdout.
static void print_version();

// ─────────────────────────────────────────────────────────────────────────────
// run_repl / run_file — stubs for future phases
// ─────────────────────────────────────────────────────────────────────────────

// We need a persistent state for the REPL
struct ExecutionState
{
    SymbolTable globals;
    VM vm;
    
    ExecutionState()
    {
        // Pre-declare stdlib in semantics
        globals.declare("print", ValueType::Any);
        globals.declare("clock", ValueType::Any);
        globals.declare("type_of", ValueType::Any);
        
        // Inject stdlib in VM
        register_stdlib(vm);
    }
};

static InterpretResult execute_source(std::string_view source, const char* name, ExecutionState& state)
{
    Lexer lexer(source, name);
    Parser parser(lexer);
    
    std::vector<std::unique_ptr<Stmt>> stmts;
    try
    {
        stmts = parser.parse();
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
    auto chunk = generator.generate(stmts);
    
    return state.vm.interpret(chunk);
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

static void run_file(std::string_view path)
{
    std::ifstream file(std::string{path});
    if (!file.is_open())
    {
        std::cerr << "Could not open file: " << path << "\n";
        exit(74); // EX_IOERR
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    ExecutionState state;
    InterpretResult result = execute_source(buffer.str(), path.data(), state);
    
    if (result == InterpretResult::CompileError) exit(65); // EX_DATAERR
    if (result == InterpretResult::RuntimeError) exit(70); // EX_SOFTWARE
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

        // Treat as source file
        run_file(arg);
        return 0;
    }

    // Unknown usage
    std::cerr << "error: unexpected arguments\n\n";
    print_usage(program_name);
    return 1;
}
