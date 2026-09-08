# Decisões de design atuais

## Produto

A Blades é uma linguagem de scripting tipada e embutível. Ela pode ser usada sozinha, mas seu principal diferencial é integrar-se a aplicações C++ por meio de funções nativas e userdata.

## Paradigma

A base é imperativa, com funções de primeira classe, closures, pattern matching e orientação a objetos por classes. Recursos funcionais podem crescer sem tornar o núcleo puramente funcional.

## Tipagem

A inferência local é combinada com anotações opcionais de parâmetros e retornos. `Unknown` representa informação ainda não resolvida; `Any` representa uma fronteira dinâmica controlada, principalmente em funções nativas.

## Mutabilidade

`let` é mutável por compatibilidade com os scripts existentes. `const` é imutável e já é verificado pelo analisador semântico. Uma futura versão poderá introduzir `mut` caso a linguagem migre para uma convenção Rust-like.

## Execução

A implementação atual usa bytecode stack-based e uma VM própria. A separação de uma IR independente fica reservada para a fase de backend nativo/WASM.

## Memória e recursos

Valores gerenciados usam objetos compartilhados. Recursos externos usam userdata com finalizador. A evolução planejada é um heap rastreável ou detector de ciclos completo para evitar ciclos entre closures e objetos.

## Módulos

Imports são resolvidos por caminho canônico relativo ao arquivo importador. Namespaces e manifesto de projeto serão adicionados antes de um gerenciador de pacotes.

## Plataforma

CMake e C++20 são usados para manter portabilidade. Raylib é uma dependência opcional; o núcleo deve funcionar em ambientes sem janela ou GPU.
