# Especificação da Linguagem Blades

A **Blades** é uma linguagem de programação procedural, estaticamente tipada, com sintaxe baseada na família C (JavaScript, Rust). Ela foi projetada para ser leve, embutível e executada através de uma Máquina Virtual (VM) de stack própria escrita em C++20.

## 1. Tipos de Dados (Primitivos)

A linguagem suporta nativamente os seguintes tipos de dados em tempo de execução:
- **Int**: Números inteiros (64-bit). Exemplo: `10`, `-42`.
- **Double**: Números de ponto flutuante (64-bit). Exemplo: `3.14`, `-0.5`.
- **Bool**: Valores booleanos (`true` ou `false`).
- **String**: Sequências de texto. Exemplo: `"Olá, Mundo!"`.
- **Nil**: Representa a ausência de valor (nulo).
- **NativeFn**: Ponteiros para funções injetadas via C++.

> O Analisador Semântico trava o tipo de uma variável na declaração. Tentativas de mudar o tipo de uma variável posteriormente resultarão em erro de compilação.

## 2. Sintaxe Básica

A sintaxe é case-sensitive. Cada instrução deve terminar com ponto e vírgula (`;`). Blocos de código são delimitados por chaves (`{` e `}`).

### 2.1 Comentários
- Comentários de linha única: `// comentário`
- Comentários de múltiplas linhas: `/* comentário */`

### 2.2 Variáveis
Variáveis são declaradas usando a palavra-chave `let`.
```js
let nome = "Blades";
let versao = 1;
let ativa = true;

// Reatribuição (o tipo deve permanecer o mesmo)
versao = 2;
```

## 3. Expressões e Operadores

### Matemáticos
- Adição: `+`
- Subtração: `-`
- Multiplicação: `*`
- Divisão: `/`

### Comparação e Lógicos
- Igualdade: `==` e `!=`
- Relacionais: `<`, `<=`, `>`, `>=`
- Lógicos: `&&` (AND), `||` (OR), `!` (NOT)

A linguagem respeita a precedência matemática convencional (PEMDAS). Parênteses `()` podem ser usados para forçar a precedência.

## 4. Estruturas de Controle

### 4.1 Condicionais (If / Else)
O corpo do `if` exige o uso de chaves, mesmo para instruções únicas.
```js
let idade = 20;

if (idade >= 18) {
    print("Maior de idade");
} else {
    print("Menor de idade");
}
```

### 4.2 Laços de Repetição (While)
```js
let i = 0;
while (i < 5) {
    print("Iteração:", i);
    i = i + 1;
}
```

## 5. Escopo e Sombreamento (Shadowing)

O escopo é delimitado lexicalmente por blocos `{}`. Variáveis declaradas dentro de um bloco não são acessíveis do lado de fora.
```js
let a = 1;
{
    let a = 2; // Sombreia (shadowing) a variável externa
    print(a);  // Imprime 2
}
print(a);      // Imprime 1
```

## 6. Funções e Biblioteca Padrão (StdLib)

Na versão atual, o sistema expõe funções essenciais diretamente através do motor (Runtime):

- `print(arg1, arg2, ...)`: Imprime os valores no terminal com uma quebra de linha.
- `clock()`: Retorna o timestamp em segundos (ponto flutuante) para medição de performance.
- `type_of(val)`: Retorna uma string identificando o tipo da variável em tempo de execução (`"int"`, `"double"`, `"bool"`, `"string"`).

*(Funções customizadas através da palavra-chave `fn` fazem parte da gramática da linguagem e estão planejadas para suporte completo no bytecode em versões futuras).*
