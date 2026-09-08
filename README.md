<div align="center">
  <h1>🗡️ Blades</h1>
  <p>Linguagem de scripting tipada, embutível e extensível, construída em C++20.</p>

  ![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
  ![License](https://img.shields.io/badge/License-MIT-green.svg)
  ![Version](https://img.shields.io/badge/Version-0.1.x-orange.svg)
</div>

## Visão geral

A Blades é uma linguagem de propósito geral executada por uma VM própria de bytecode. Ela combina uma sintaxe familiar com tipagem progressiva, funções, closures, classes, coleções, imports e integração nativa em C++.

O núcleo funciona sem Raylib. O módulo gráfico opcional adiciona janela, câmera livre, vetores, cores, desenho 3D e partículas para protótipos, visualizações e jogos.

## Recursos atuais

- Lexer, parser recursivo e recuperação de erros de sintaxe.
- Análise semântica com escopos, `let`, `const`, anotações de tipos e assinaturas de funções.
- Tipos `int`, `float`, `number`, `bool`, `string`, `nil`, `array`, `dict`, `any` e objetos da VM.
- Funções nomeadas, funções anônimas, closures, recursão e retorno tipado.
- Classes, métodos, herança e `super`.
- Arrays, dicionários, `match`, `for`, `while`, `break`, `continue` e `yield`.
- Imports relativos ao arquivo que os utiliza.
- REPL, verificação sem execução e disassembly do bytecode.
- Funções nativas C++ e valores nativos com destrutor seguro.
- Raylib opcional com câmera `CAMERA_FREE`, `draw_cube`, `draw_plane`, `draw_sphere` e sistema de partículas.

## Exemplo de linguagem

```blades
const nome: string = "Blades";

fn fibonacci(n: int) -> int {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

for (let i: int = 0; i < 8; i = i + 1) {
    print(nome, ": ", fibonacci(i));
}
```

## Exemplo 3D

O arquivo [`examples/fps_table.bl`](examples/fps_table.bl) cria uma mesa usando cubos e plano, com navegação em primeira pessoa por teclado e mouse.

Controles: `WASD` movimenta a câmera, o mouse altera a direção de visão e `ESC` fecha a janela.

```blades
let camera = create_camera_3d(
    vec3(8.0, 5.5, 9.0),
    vec3(0.0, 2.5, 0.0),
    vec3(0.0, 1.0, 0.0),
    60.0,
    CAMERA_PERSPECTIVE
);

while (!window_should_close()) {
    update_camera(camera, CAMERA_FREE);
    begin_drawing();
    begin_mode_3d(camera);
    draw_cube(vec3(0, 3.4, 0), vec3(6, 0.45, 3.2), color(145, 82, 42, 255));
    end_mode_3d();
    end_drawing();
}
```

## Compilação

### Requisitos

- CMake 3.20 ou superior.
- Compilador com C++20: MSVC, GCC ou Clang.
- Raylib 5.0 é baixada automaticamente apenas quando o módulo 3D está habilitado.

### Windows com Visual Studio

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -DBLADES_ENABLE_RAYLIB=ON
cmake --build build --config Debug
```

### Linux, macOS ou MinGW

```bash
cmake -S . -B build -DBLADES_ENABLE_RAYLIB=ON
cmake --build build
```

Para compilar somente o núcleo, sem Raylib:

```bash
cmake -S . -B build-core -DBLADES_ENABLE_RAYLIB=OFF
cmake --build build-core
```

## Uso da CLI

No Windows com Visual Studio:

```powershell
# REPL interativo
.\build\bin\Debug\blades.exe

# Executar um arquivo
.\build\bin\Debug\blades.exe examples/hello.bl

# Verificar sintaxe e tipos sem executar o programa
.\build\bin\Debug\blades.exe --check examples/fps_table.bl

# Exibir o bytecode gerado
.\build\bin\Debug\blades.exe --disassemble examples/hello.bl

# Executar o exemplo 3D
.\build\bin\Debug\blades.exe examples/fps_table.bl
```

Em builds Unix, substitua o caminho pelo binário gerado em `build/bin/blades` ou `build/blades`, conforme o gerador usado.

## Organização do projeto

```text
src/                    compilador, VM e biblioteca nativa
examples/               programas de exemplo
docs/                   especificação, gramática e arquitetura
tools/vscode-blades/    extensão de sintaxe e execução para VS Code
```

## Documentação

- [Especificação da linguagem](docs/language_spec.md)
- [Gramática](docs/grammar.md)
- [Arquitetura](docs/architecture.md)
- [Decisões de projeto](docs/design_decisions.md)

## Contribuição

Pull requests são bem-vindos. As próximas áreas planejadas incluem biblioteca padrão modular, namespaces, package manager, formatter, Language Server, debugger e backend WebAssembly.

## Licença

Este projeto está disponível sob a licença [MIT](LICENSE).
