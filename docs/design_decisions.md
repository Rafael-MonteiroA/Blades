# Blades — Design Decisions

Registro permanente das 8 decisões de design da linguagem, tomadas antes da Fase 0.

---

## 1. Paradigma: Imperativo + Funcional Leve

**Decisão:** Base imperativa/procedural com elementos funcionais leves.

**O que isso significa:**
- Funções como cidadãs de primeira classe (closures, higher-order functions)
- Pattern matching (match expressions)
- Imutabilidade por padrão com `mut` para mutabilidade explícita
- Composição via structs + traits em vez de herança clássica
- Sem classes com herança, sem métodos virtuais por padrão

**Alternativas descartadas:**
- OOP puro: complexidade de vtables e herança desde o início
- Funcional puro: garbage collector quase obrigatório; difícil mapear para hardware
- Multiparadigma completo: risco de incoerência de design; complexidade do compilador

---

## 2. Tipagem: Estática, Forte, com Inferência Local

**Decisão:** Sistema de tipos estático e forte, com inferência restrita ao escopo local de funções.

**O que isso significa:**
- Erros de tipo detectados em tempo de compilação (Fase 3 — Semantic Analysis)
- Sem conversões implícitas entre tipos numéricos (i32 ≠ f64; cast explícito necessário)
- `let x = 42` → o tipo é inferido como `i32` dentro da função
- `let x: f64 = 42` → declaração explícita necessária quando a inferência é ambígua
- Sem inferência global (Hindley-Milner); mantém o compilador compreensível

**Alternativas descartadas:**
- Dinâmico: sem type checker educativo; performance ruim; erros tardios
- Inferência global: correto mas extremamente complexo de implementar (algoritmo W/unification)

---

## 3. Execução: Bytecode + VM Própria

**Decisão:** Compilar para bytecode de uma representação intermediária (IR) própria, executado por uma VM stack-based/register-based custom.

**O que isso significa:**
- Pipeline: Source → Lexer → Parser → AST → Semantic → IR → Bytecode → VM
- A IR permite otimizações simples antes de gerar bytecode
- A VM é implementada do zero (no estilo de CPython, Lua)
- Backend nativo (x86_64) é possível futuramente usando a mesma IR como entrada

**Alternativas descartadas:**
- Compilação nativa direta: semanas apenas para rodar o primeiro "hello world" (codegen x86 + linking + ABI)
- Tree-walking interpreter: lento, sem caminho natural para otimização ou compilação
- LLVM/JVM como backend: viola a filosofia "do zero"

---

## 4. Gerenciamento de Memória: Reference Counting + Detector de Ciclos

**Decisão:** Contagem de referência automática com detector de ciclos (trial deletion / mark-and-sweep parcial para ciclos).

**O que isso significa:**
- Cada objeto tem um contador de referências; libera-se quando conta chega a zero
- Determinístico (destruidores chamados imediatamente, sem pausas de GC)
- Detector de ciclos rodará periodicamente para objetos suspeitos
- O programador Blades não gerencia memória manualmente

**Alternativas descartadas:**
- Mark-and-sweep GC: mais complexo de implementar; pausas não-determinísticas
- Ownership/borrow checker: fascinante, mas é quase um projeto separado em complexidade (Fase 9+)
- Manual (malloc/free): inadequado para linguagem scripting moderna; muito bug-prone

---

## 5. Propósito: Scripting Embarcável

**Decisão:** Linguagem de scripting projetada para ser embarcada em aplicações C++ (ex: Zenith Engine), com uso standalone também suportado.

**O que isso significa:**
- API C++ de embedding para instanciar e controlar a VM de fora
- FFI (Foreign Function Interface) com C/C++ para expor funções do host
- Escopo bem definido evita "feature creep"
- Runtime e stdlib mínimas (sem necessidade de sistema de pacotes)

**Alternativas descartadas:**
- Propósito geral: escopo infinito; risco de nunca ter um produto funcional
- Linguagem de sistemas: requer controle fino de memória e baixo nível desde o início
- Puramente experimental: sem objetivo concreto, difícil manter motivação

---

## 6. Sintaxe: Rust-Inspired, Simplificada

**Decisão:** Sintaxe inspirada em Rust com simplificações e influências de Go/Kotlin.

**Características principais:**
- Sem ponto-e-vírgula obrigatório (newline como terminador)
- Chaves obrigatórias para todos os blocos (sem one-liners sem chaves)
- `let` para imutável, `mut` para mutável
- `fn` para funções, `struct` para tipos compostos, `impl` para métodos
- `->` para tipo de retorno
- `match` para pattern matching
- Dois pontos para anotação de tipo (`x: i32`)

**Exemplo:**
```blades
fn fibonacci(n: i32) -> i32 {
    match n {
        0 => return 0
        1 => return 1
        _ => return fibonacci(n - 1) + fibonacci(n - 2)
    }
}

let result = fibonacci(10)
print(result)
```

**Alternativas descartadas:**
- C-like com `;` e `()` obrigatórios em condicionais: "mais do mesmo"; sem identidade
- Python-like com indentação: parser de indentação é complexo e ambíguo
- Sintaxe completamente original: risco de inconsistência sem inspiração sólida

---

## 7. Plataforma: Windows Principal, Design Cross-Platform

**Decisão:** Alvo primário Windows (ambiente do desenvolvedor), com arquitetura que suporta portabilidade futura.

**O que isso significa:**
- Abstrações de I/O no `common/` (sem chamadas de sistema diretas no compilador)
- CMake como sistema de build (suporta MSVC, GCC, Clang)
- Sem código `#ifdef _WIN32` no compilador core; apenas no runtime quando necessário
- Testado com MSVC 2022 e GCC/Clang via WSL/GitHub Actions futuramente

---

## 8. Linguagem de Implementação: C++20 com CMake

**Decisão:** C++20 para implementar o compilador e a VM. CMake como sistema de build.

**Features C++20 usadas:**
- `std::variant` + `std::visit` para nós da AST e valores da VM
- `std::unique_ptr` para ownership de nós da árvore
- `std::string_view` para tokenização eficiente sem cópias
- `std::span` para slices de arrays sem cópia
- Concepts (para genericidade no sistema de tipos)
- `constexpr` agressivo para constantes do compilador

**Alternativas descartadas:**
- C: verboso; sem RAII; sem `std::variant` para unions type-safe
- Rust: curva de aprendizado adicional; borrow checker pode atrasar o projeto
- Go: GC; runtime pesado; menos controle fino

---

*Documento criado em: 2026-08-22*
*Última atualização: Fase 0 — Fundação de Engenharia*
