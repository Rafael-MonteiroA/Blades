<div align="center">
  <h1>🗡️ Blades</h1>
  <p>Uma linguagem de programação elegante, leve e incrivelmente rápida, construída do zero em C++20.</p>
  
  ![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
  ![License](https://img.shields.io/badge/License-MIT-green.svg)
  ![Status](https://img.shields.io/badge/Status-Stable-brightgreen.svg)
</div>

## O que é a Blades?

A **Blades** é uma linguagem de script embeddável, compilada e fortemente tipada (com inferência) que roda sobre uma Máquina Virtual (VM) de stack própria. 

O projeto foi construído inteiramente do zero, passando por todas as etapas fundamentais da criação de um compilador clássico: **Lexer**, **Parser**, **Análise Semântica (Type Checker)**, **Geração de Código (IR)** e uma **Máquina Virtual (Backend)**.

### ✨ Principais Recursos

- **Sintaxe Familiar:** C-family (JS/Rust/Swift) para você não ter que reaprender a programar.
- **Tipagem Segura:** Uma vez declarada, o verificador semântico da Blades garante estaticamente que a variável mantenha seu tipo na reatribuição.
- **Máquina Virtual (VM) Rápida:** Baseada em execução de bytecode com base em stack (pilha).
- **Sem Dependências Externas pesadas:** Escrito em C++20 moderno com `std::variant`, priorizando performance crua e segurança de memória.
- **Interoperabilidade com C++:** Capacidade de injetar Funções Nativas do C++ dentro dos scripts nativamente de forma quase instantânea.

## Exemplo de Código

```js
// Definindo funções recursivas
fn fibonacci(n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Declarando arrays e usando laços for
let limites = [3, 5, 7];
print("Calculando Fibonacci para os limites:", limites);

for (let i = 0; i < len(limites); i = i + 1) {
    print("Fibonacci(", limites[i], ") = ", fibonacci(limites[i]));
}

// Inspecionando tipos dinamicamente
let tipo = type_of(limites);
print("A variavel 'limites' eh do tipo: ", tipo);
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

### Usando a Linguagem

O binário final gera ferramentas incríveis direto para o seu console:

```powershell
# 1. Inicia o modo interativo (REPL) - Digite e veja os resultados na hora!
.\build\bin\Debug\blades.exe

# 2. Executa um script escrito em um arquivo
.\build\bin\Debug\blades.exe meu_codigo.bl
```

### Rodando a Suíte de Testes
O projeto usa um framework de testes customizado para atestar todos os stages do compilador.
```powershell
.\build\bin\Debug\blades_tests.exe
```

## Como Contribuir

Se sinta à vontade para abrir pull requests! 
Atualmente, as maiores necessidades do interpretador são:
- Tipos de dados complexos: Dicionários (Hash Maps).
- Orientação a Objetos (Classes, Métodos e Instâncias).
- Pacotes de extensão (importação de módulos).
