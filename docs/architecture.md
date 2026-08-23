# Blades — Architecture Overview

Visão geral da arquitetura do compilador e VM da linguagem Blades.

---

## Pipeline Completo

```
┌─────────────────────────────────────────────────────────────────┐
│                        Source File (.bl)                         │
└───────────────────────────────┬─────────────────────────────────┘
                                │ raw text
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Phase 1 — LEXER                              │
│                                                                  │
│  Input:  &str (source text)                                      │
│  Output: Vec<Token>                                              │
│                                                                  │
│  Tokenizes the source into keywords, identifiers, literals,      │
│  operators, and punctuation. Attaches SourceSpan to each token.  │
└───────────────────────────────┬─────────────────────────────────┘
                                │ token stream
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Phase 2 — PARSER & AST                       │
│                                                                  │
│  Input:  Vec<Token>                                              │
│  Output: AST (tree of AstNode)                                   │
│                                                                  │
│  Recursive-descent parser (Pratt parsing for expressions).       │
│  Produces a typed AST with SourceSpan on every node.             │
└───────────────────────────────┬─────────────────────────────────┘
                                │ untyped AST
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Phase 3 — SEMANTIC ANALYSIS                     │
│                                                                  │
│  Input:  untyped AST                                             │
│  Output: typed AST + symbol table                                │
│                                                                  │
│  Name resolution, scope analysis, type inference (local),        │
│  type checking. Emits rich diagnostics with source spans.        │
└───────────────────────────────┬─────────────────────────────────┘
                                │ typed AST
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│               Phase 4 — INTERMEDIATE REPRESENTATION (IR)         │
│                                                                  │
│  Input:  typed AST                                               │
│  Output: Blades IR (3-address code, SSA-inspired)                │
│                                                                  │
│  Lowers the AST to a flat, linear representation amenable        │
│  to optimization and multiple backends. Simple optimizations      │
│  (constant folding, dead code elimination) applied here.         │
└───────────────────────────────┬─────────────────────────────────┘
                                │ IR
                    ┌───────────┴───────────┐
                    │                       │
                    ▼                       ▼ (future)
┌───────────────────────────┐  ┌───────────────────────────────┐
│ Phase 5 — BYTECODE CODEGEN │  │   Phase 9+ — NATIVE CODEGEN   │
│                           │  │                               │
│ Input:  IR                │  │ Input:  IR                    │
│ Output: Blades bytecode   │  │ Output: x86_64 machine code   │
│                           │  │                               │
│ Emits compact bytecode    │  │ (future phase, uses same IR)  │
│ for the Blades VM.        │  └───────────────────────────────┘
└─────────────┬─────────────┘
              │ bytecode
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Phase 5 — VM RUNTIME                         │
│                                                                  │
│  Input:  Blades bytecode                                         │
│  Output: program execution                                       │
│                                                                  │
│  Stack-based or register-based virtual machine.                  │
│  Manages call stack, values, reference counting, heap.           │
│  Embeddable via C++ API.                                         │
└─────────────────────────────────────────────────────────────────┘
```

---

## Module Map (por fase)

```
src/
├── common/                 ← Phase 0: types, Result, SourceLocation
├── lexer/                  ← Phase 1: Lexer, Token, TokenKind
├── parser/                 ← Phase 2: Parser, AST nodes
├── sema/                   ← Phase 3: TypeChecker, SymbolTable, Scope
├── ir/                     ← Phase 4: IR builder, IR instructions
├── codegen/                ← Phase 5: BytecodeEmitter
├── vm/                     ← Phase 5: VM, CallStack, Heap
├── runtime/                ← Phase 6: stdlib (print, math, collections)
└── tools/                  ← Phase 7: REPL, Formatter
```

---

## Key Design Principles

1. **Each phase is independent** — a phase only depends on the output of the previous phase, never skips layers.
2. **Errors carry source spans** — every diagnostic points to the exact location in source using `SourceSpan`.
3. **No exceptions in the pipeline** — all errors propagate via `Result<T, E>`.
4. **One responsibility per file** — each `.cpp`/`.hpp` pair owns a single, well-defined concept.
5. **Progressive disclosure** — the IR is designed from the start to support both a bytecode backend (Phase 5) and a future native backend (Phase 9+).

---

*Last updated: Phase 0 — Engineering Foundation*
