# Especificação atual da linguagem Blades

Este documento descreve a implementação atual da Blades 0.1.x. A linguagem é um scripting embutível, compilado para bytecode e executado por uma VM própria. Raylib e os recursos 3D são opcionais na build.

## Sintaxe

A linguagem é case-sensitive e usa `;` para terminar instruções. Blocos usam `{}`. Há comentários `//` e `/* ... */`.

```blades
let nome = "Blades";
const versao = 1;

if (versao == 1) {
    print(nome);
}
```

`let` cria uma ligação mutável. `const` cria uma ligação que não pode ser reatribuída. Uma anotação opcional pode declarar o tipo esperado:

```blades
let contador: int = 0;
const nome: string = "Blades";
```

## Tipos

Os tipos primitivos são `Int`, `Float`, `Bool`, `String` e `Nil`. A VM também possui arrays, dicionários, funções, closures, classes, instâncias, vetores, cores e userdata.

Anotações de parâmetros e retorno são opcionais:

```blades
fn add(a: int, b: int) -> int {
    return a + b;
}
```

Tipos aceitos nas anotações: `int`, `i32`, `i64`, `float`, `f32`, `f64`, `double`, `number`, `bool`, `string`, `str`, `nil`, `void`, `array`, `list`, `dict`, `map` e `any`. `number` aceita inteiros e reais.

O analisador verifica aridade, tipos de argumentos, atribuições, condições booleanas e tipos de retorno. `any` e valores vindos de funções nativas são tratados como fronteiras dinâmicas.

## Funções e classes

```blades
class Pessoa {
    fn init(nome) {
        this.nome = nome;
    }

    fn falar() {
        print(this.nome);
    }
}

let pessoa = Pessoa("Rafael");
pessoa.falar();
```

Closures são criadas com `fn(...) { ... }`. Classes podem herdar usando `<` e acessar métodos da classe base com `super.metodo()`.

## Coleções

```blades
let valores = [1, 2, 3];
let pessoa = { "nome": "Rafael", "idade": 25 };

print(valores[0]);
print(pessoa["nome"]);
```

Arrays possuem a propriedade `length`. Dicionários usam chaves string.

## Controle de fluxo

Há `if/else`, `while`, `for`, `break`, `continue`, `return` e expressões `match`:

```blades
let resultado = match (valor) {
    1 | 2 => "baixo",
    _ if valor < 10 => "medio",
    _ => "alto"
};
```

## Biblioteca padrão atual

`print`, `clock`, `type_of`, `random`, `input`, `len`, funções matemáticas, leitura/escrita de texto, arrays, `vec2`, `vec3` e `color`.

Com `BLADES_ENABLE_RAYLIB=ON`, ficam disponíveis janela, desenho, câmera 3D, input e sistema de partículas.

O exemplo `examples/fps_table.bl` demonstra uma mesa montada com `draw_cube` e `draw_plane`, usando `CAMERA_FREE` para navegação em primeira pessoa com teclado e mouse.

## Módulos

Imports usam caminhos relativos ao arquivo que importa:

```blades
import "util.bl";
```

O carregamento é feito uma vez por caminho canônico. Namespaces, manifesto de projeto e pacote de módulos são próximos passos do projeto.
