# Arquitetura atual da Blades

## Pipeline

```text
arquivo .bl
   ↓
Lexer → tokens com SourceSpan
   ↓
Parser → AST
   ↓
SemanticAnalyzer → escopos, tipos e assinaturas
   ↓
IRGenerator → bytecode stack-based
   ↓
VM → execução, closures, classes e chamadas nativas
```

O bytecode atual é deliberadamente próximo da VM. Uma IR independente será adicionada antes de um backend nativo ou WebAssembly.

## Componentes

```text
src/common     tipos, Result e localização de fonte
src/compiler   tokens, lexer, parser, AST, tipos e geração de bytecode
src/backend    Value, closures, frames, VM e execução
src/runtime    funções nativas e recursos padrão
```

Raylib está protegido por `BLADES_ENABLE_RAYLIB` e `BLADES_HAS_RAYLIB`. Com a opção desligada, o núcleo compila e executa sem dependência gráfica.

## Regras de fronteira

- O compilador não deve depender de Raylib.
- Funções nativas devem validar aridade e argumentos.
- Recursos externos devem possuir finalizador.
- A VM deve converter falhas nativas em erros de runtime.
- O CLI é responsável por carregamento de módulos e apresentação de diagnósticos.

## Próxima evolução

```text
AST tipada → HIR tipada → IR independente → bytecode VM
                                      ├── backend WASM
                                      └── backend nativo
```
