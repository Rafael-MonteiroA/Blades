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
#include <filesystem>

#include "version.hpp"

#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/semantic_analyzer.hpp"
#include "compiler/ir.hpp"
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
    std::vector<std::unique_ptr<std::string>> loaded_filenames;
    SemanticAnalyzer::FunctionSignatureTable function_signatures;
    
    ExecutionState()
    {
        // Pre-declare stdlib in semantics
        globals.declare("print", ValueType::Any);
        globals.declare("clock", ValueType::Any);
        globals.declare("type_of", ValueType::Any);
        globals.declare("random", ValueType::Any);
        globals.declare("input", ValueType::Any);
        globals.declare("len", ValueType::Any);
        
        globals.declare("sin", ValueType::Any);
        globals.declare("cos", ValueType::Any);
        globals.declare("tan", ValueType::Any);
        globals.declare("sqrt", ValueType::Any);
        globals.declare("abs", ValueType::Any);
        globals.declare("pow", ValueType::Any);
        globals.declare("dot", ValueType::Any);
        globals.declare("array_push", ValueType::Any);
        globals.declare("array_pop", ValueType::Any);
        globals.declare("read_text", ValueType::Any);
        globals.declare("write_text", ValueType::Any);
        globals.declare("vec2", ValueType::Any);
        globals.declare("vec3", ValueType::Any);
        globals.declare("color", ValueType::Any);

#ifdef BLADES_HAS_RAYLIB
        globals.declare("init_window", ValueType::Any);
        globals.declare("close_window", ValueType::Any);
        globals.declare("window_should_close", ValueType::Any);
        globals.declare("begin_drawing", ValueType::Any);
        globals.declare("end_drawing", ValueType::Any);
        globals.declare("clear_background", ValueType::Any);
        globals.declare("create_camera_3d", ValueType::Any);
        globals.declare("begin_mode_3d", ValueType::Any);
        globals.declare("end_mode_3d", ValueType::Any);
        globals.declare("update_camera", ValueType::Any);
        globals.declare("draw_sphere", ValueType::Any);
        globals.declare("draw_cube", ValueType::Any);
        globals.declare("draw_plane", ValueType::Any);
        globals.declare("is_key_down", ValueType::Any);
        globals.declare("is_key_pressed", ValueType::Any);
        globals.declare("set_target_fps", ValueType::Any);
        globals.declare("disable_cursor", ValueType::Any);
        
        globals.declare("create_particle_system", ValueType::Any);
        globals.declare("update_particle_system", ValueType::Any);
        globals.declare("draw_particle_system", ValueType::Any);
        globals.declare("add_particles", ValueType::Any);
        globals.declare("remove_particles", ValueType::Any);
        
        globals.declare("KEY_UP", ValueType::Any);
        globals.declare("KEY_DOWN", ValueType::Any);
        globals.declare("CAMERA_FREE", ValueType::Any);
        globals.declare("CAMERA_PERSPECTIVE", ValueType::Any);
#endif

        // Signatures for native functions that have a stable contract. Calls
        // to variadic or overloaded helpers remain dynamic and are checked by
        // the runtime boundary.
        function_signatures["sin"] = {{ValueType::Number}, ValueType::Float};
        function_signatures["cos"] = {{ValueType::Number}, ValueType::Float};
        function_signatures["tan"] = {{ValueType::Number}, ValueType::Float};
        function_signatures["sqrt"] = {{ValueType::Number}, ValueType::Float};
        function_signatures["abs"] = {{ValueType::Number}, ValueType::Float};
        function_signatures["pow"] = {{ValueType::Number, ValueType::Number}, ValueType::Float};
        function_signatures["array_push"] = {{ValueType::Array, ValueType::Any}, ValueType::Nil};
        function_signatures["array_pop"] = {{ValueType::Array}, ValueType::Any};
        function_signatures["read_text"] = {{ValueType::String}, ValueType::String};
        function_signatures["write_text"] = {{ValueType::String, ValueType::String}, ValueType::Bool};
        
        // Inject modules in VM
        register_stdlib(vm);
    }
};

static std::vector<std::unique_ptr<Stmt>> link_ast(std::vector<std::unique_ptr<Stmt>> stmts,
                                                   ExecutionState& state,
                                                   const std::filesystem::path& base_dir)
{
    std::vector<std::unique_ptr<Stmt>> linked;
    for (auto& stmt : stmts)
    {
        if (auto* import_stmt = dynamic_cast<ImportStmt*>(stmt.get()))
        {
            std::string path(import_stmt->path.lexeme);
            if (path.length() >= 2 && path.front() == '"' && path.back() == '"')
                path = path.substr(1, path.length() - 2);

            std::filesystem::path requested(path);
            if (requested.is_relative()) requested = base_dir / requested;

            std::error_code fs_error;
            std::filesystem::path resolved = std::filesystem::weakly_canonical(requested, fs_error);
            if (fs_error) resolved = std::filesystem::absolute(requested, fs_error);
            const std::string resolved_name = resolved.lexically_normal().string();

            if (state.imported_files.find(resolved_name) == state.imported_files.end())
            {
                state.imported_files.insert(resolved_name);
                
                std::ifstream file(resolved);
                if (!file.is_open())
                {
                    std::cerr << "Linker Error: Could not open module '" << resolved_name << "'\n";
                    throw ParseError("Module not found");
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                
                auto source_str = std::make_unique<std::string>(buffer.str());
                std::string_view source_view = *source_str;
                state.loaded_sources.push_back(std::move(source_str));
                auto filename = std::make_unique<std::string>(resolved_name);
                std::string_view filename_view = *filename;
                state.loaded_filenames.push_back(std::move(filename));
                
                Lexer lexer(source_view, filename_view);
                Parser parser(lexer);
                auto module_stmts = parser.parse();
                if (parser.had_error())
                    throw ParseError("Module contains syntax errors");
                
                auto linked_module = link_ast(std::move(module_stmts), state, resolved.parent_path());
                
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

static InterpretResult compile_source(std::string_view source,
                                      const char* name,
                                      ExecutionState& state,
                                      std::shared_ptr<ObjFunction>& function)
{
    Lexer lexer(source, name);
    Parser parser(lexer);
    
    std::vector<std::unique_ptr<Stmt>> stmts;
    try
    {
        stmts = parser.parse();
        if (parser.had_error())
            return InterpretResult::CompileError;
        const std::filesystem::path source_path(name ? name : "<stdin>");
        const std::filesystem::path base_dir = source_path.has_parent_path()
            ? source_path.parent_path()
            : std::filesystem::current_path();
        stmts = link_ast(std::move(stmts), state, base_dir);
    }
    catch (const ParseError& e)
    {
        std::cerr << e.what() << "\n";
        return InterpretResult::CompileError;
    }
    
    try
    {
        SemanticAnalyzer semantic(state.globals, &state.function_signatures);
        semantic.analyze(stmts);
    }
    catch (const SemanticError& e)
    {
        std::cerr << e.what() << "\n";
        return InterpretResult::CompileError;
    }
    catch (const std::exception& e)
    {
        std::cerr << "C++ Exception: " << e.what() << "\n";
        return InterpretResult::RuntimeError;
    }
    
    try
    {
        IRGenerator generator;
        function = generator.generate(stmts, name ? std::string_view{name} : std::string_view{});
    }
    catch (const std::exception& e)
    {
        std::cerr << "Code generation error: " << e.what() << "\n";
        return InterpretResult::CompileError;
    }

    return InterpretResult::Ok;
}

static InterpretResult execute_source(std::string_view source, const char* name, ExecutionState& state)
{
    std::shared_ptr<ObjFunction> function;
    const InterpretResult compile_result = compile_source(source, name, state, function);
    if (compile_result != InterpretResult::Ok)
        return compile_result;

    InterpretResult result = state.vm.interpret(function);
    while (result == InterpretResult::Yield)
    {
        result = state.vm.resume(state.vm.get_current_fiber());
    }
    
    return result;
}

static bool read_source_file(std::string_view filename, std::string& source)
{
    std::ifstream file(std::string{filename});
    if (!file.is_open())
        return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    source = buffer.str();
    return true;
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
        << "  " << program_name << " --check <file.bl>       Check syntax and types without running\n"
        << "  " << program_name << " --disassemble <file.bl> Compile and print bytecode\n"
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

        std::string source;
        if (!read_source_file(arg, source))
        {
            std::cerr << "Could not open file: " << arg << "\n";
            return 74; // EX_IOERR
        }

        blades::ExecutionState state;

        // Evaluate the script (no engine ECS)
        InterpretResult result = execute_source(source, arg.data(), state);
        
        return result == InterpretResult::Ok ? 0 : 1;
    }

    if (argc == 3)
    {
        std::string_view command = args[1];
        std::string_view filename = args[2];
        if (command == "--check" || command == "--disassemble" || command == "--dump-bytecode")
        {
            std::string source;
            if (!read_source_file(filename, source))
            {
                std::cerr << "Could not open file: " << filename << "\n";
                return 74; // EX_IOERR
            }

            blades::ExecutionState state;
            std::shared_ptr<ObjFunction> function;
            const InterpretResult result = compile_source(source, filename.data(), state, function);
            if (result != InterpretResult::Ok)
                return 1;

            if (command == "--disassemble" || command == "--dump-bytecode")
                std::cout << disassemble_chunk(function->chunk, std::string{filename});

            return 0;
        }
    }

    // Unknown usage
    std::cerr << "error: unexpected arguments\n\n";
    print_usage(program_name);
    return 1;
}
