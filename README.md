<div align="center">
  <h1>🗡️ Blades</h1>
  <p>Uma linguagem de scripting tipada, embutível e extensível, construída do zero em C++20.</p>
  
  ![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
  ![License](https://img.shields.io/badge/License-MIT-green.svg)
  ![Status](https://img.shields.io/badge/Status-Stable-brightgreen.svg)
</div>

## O que é a Blades?

A **Blades** é uma linguagem de scripting embutível, compilada para bytecode e executada por uma Máquina Virtual (VM) própria. O núcleo pode ser compilado sem Raylib; os recursos gráficos e 3D são opcionais.

O projeto foi construído inteiramente do zero, passando por todas as etapas fundamentais da criação de um compilador clássico: **Lexer**, **Parser**, **Análise Semântica (Type Checker)**, **Geração de Código (IR)** e uma **Máquina Virtual (Backend)**.

### ✨ Principais Recursos

- **Sintaxe Familiar:** C-family (JS/Rust/Swift) para você não ter que reaprender a programar.
- **Tipagem Progressivamente Segura:** Inferência local, anotações de tipos, assinaturas de funções e verificação de retornos.
- **Máquina Virtual (VM) Rápida:** Baseada em execução de bytecode com base em stack (pilha).
- **Sem Dependências Externas pesadas:** Escrito em C++20 moderno com `std::variant`, priorizando performance crua e segurança de memória.
- **Interoperabilidade com C++:** Capacidade de injetar Funções Nativas do C++ dentro dos scripts nativamente de forma quase instantânea.
- **Módulo 3D opcional:** Raylib, câmera, input, vetores, cores e simulações de partículas.

## Exemplo de Código

```js
// Orientação a Objetos
class Pessoa {
    fn init(n) { this.nome = n; }
    fn falar() { print("Olá, eu sou ", this.nome); }
}

let p = Pessoa("Blades");
p.falar();

// Dicionários e Funções Recursivas
let infos = { "versao": 1.0, "limites": [3, 5, 7] };

fn fibonacci(n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

for (let i = 0; i < len(infos["limites"]); i = i + 1) {
    print("Fibonacci(", infos["limites"][i], ") = ", fibonacci(infos["limites"][i]));
}
```

## Arquitetura do Compilador

A compilação de um script `.bl` passa pelo seguinte pipeline interno:

1. `Lexer`: Lê o arquivo bruto e o transforma em Tokens.
2. `Parser`: Constrói a Árvore Sintática Abstrata (AST) validando a gramática estrutural.
3. `SemanticAnalyzer`: Vasculha a AST, resolve o escopo e as tipagens (Type Checking).
4. `IRGenerator`: Converte a AST perfeitamente tipada num Bytecode linear (Opcodes).
5. `VM`: Desempacota e executa as instruções no processador.

Para a documentação completa de sintaxe, consulte o [Manual da Linguagem](docs/language_spec.md).

## Como Instalar e Rodar

### Requisitos
- CMake ≥ 3.20
- Compilador C++20 (MSVC 2019+, GCC 10+, Clang 12+)

### Compilando o Projeto

No Windows usando PowerShell:
```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

Para compilar apenas o núcleo, sem baixar Raylib:

```powershell
cmake -B build-core -DBLADES_ENABLE_RAYLIB=OFF
cmake --build build-core --config Debug
```

### Usando a Linguagem

O binário final gera ferramentas incríveis direto para o seu console:

```powershell
# 1. Inicia o modo interativo (REPL) - Digite e veja os resultados na hora!
.\build\bin\Debug\blades.exe

# 2. Executa um script escrito em um arquivo
.\build\bin\Debug\blades.exe meu_codigo.bl

# 3. Verifica sintaxe e tipos sem executar efeitos colaterais
.\build\bin\Debug\blades.exe --check meu_codigo.bl

# 4. Inspeciona o bytecode gerado para depuração
.\build\bin\Debug\blades.exe --disassemble meu_codigo.bl

# 5. Exemplo 3D com câmera livre (requer BLADES_ENABLE_RAYLIB=ON)
.\build\bin\Debug\blades.exe examples/fps_table.bl
```

### Rodando a Suíte de Testes
Os testes são registrados no CTest e cobrem execução, `match`, imports, tipos e recuperação de erros.
```powershell
ctest --test-dir build -C Debug --output-on-failure
```

## Como Contribuir

Se sinta à vontade para abrir pull requests! 
- Biblioteca padrão multiplataforma (`std.fs`, `std.json`, `std.collections`).
- Language Server, formatter e debugger.
- Backend WebAssembly/nativo usando uma IR independente da VM.
