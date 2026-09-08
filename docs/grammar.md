# Gramática EBNF da Blades 0.1.x

```ebnf
program        = { declaration } , EOF ;
declaration    = import_decl | class_decl | function_decl | binding_decl | statement ;
import_decl    = "import" , string , ";" ;
binding_decl   = ( "let" | "const" ) , identifier , [ ":" , identifier ] , [ "=" , expression ] , ";" ;
function_decl  = "fn" , identifier , "(" , [ parameters ] , ")" ,
                 [ "->" , identifier ] , block ;
parameters     = parameter , { "," , parameter } ;
parameter      = identifier , [ ":" , identifier ] ;
class_decl     = "class" , identifier , [ "<" , identifier ] , "{" ,
                 { function_decl } , "}" ;
statement      = if_stmt | while_stmt | for_stmt | return_stmt |
                 break_stmt | continue_stmt | block | expression_stmt ;
if_stmt        = "if" , "(" , expression , ")" , statement ,
                 [ "else" , statement ] ;
while_stmt     = "while" , "(" , expression , ")" , statement ;
for_stmt       = "for" , "(" , [ binding_decl | expression_stmt ] ,
                 [ expression ] , ";" , [ expression ] , ")" , statement ;
return_stmt    = "return" , [ expression ] , ";" ;
break_stmt     = "break" , ";" ;
continue_stmt  = "continue" , ";" ;
block          = "{" , { declaration } , "}" ;
expression_stmt = expression , ";" ;
expression     = assignment | "match" , "(" , expression , ")" , match_body ;
assignment     = logical_or , [ ( "=" | "+=" | "-=" | "*=" | "/=" | "%=" ) , assignment ] ;
logical_or     = logical_and , { ( "or" | "||" ) , logical_and } ;
logical_and    = equality , { ( "and" | "&&" ) , equality } ;
equality       = comparison , { ( "==" | "!=" ) , comparison } ;
comparison     = term , { ( "<" | "<=" | ">" | ">=" ) , term } ;
term           = factor , { ( "+" | "-" ) , factor } ;
factor         = unary , { ( "*" | "/" | "%" ) , unary } ;
unary          = [ "!" | "-" | "~" ] , call ;
call           = primary , { "(" , [ arguments ] , ")" | "[" , expression , "]" | "." , identifier } ;
arguments      = expression , { "," , expression } ;
primary        = literal | identifier | "this" | "super" , "." , identifier |
                 "fn" , "(" , [ parameters ] , ")" , block |
                 "(" , expression , ")" | array | dictionary ;
array          = "[" , [ arguments ] , "]" ;
dictionary     = "{" , [ string , ":" , expression , { "," , string , ":" , expression } ] , "}" ;
match_body     = "{" , match_arm , { "," , match_arm } , [ "," ] , "}" ;
match_arm      = ( "_" | expression , { "|" , expression } ) ,
                 [ "if" , expression ] , "=>" , ( expression | block ) ;
```

O lexer também reconhece números inteiros e reais, strings com escapes, f-strings e comentários de linha/bloco. Tipos predefinidos incluem `int`, `float`, `number`, `bool`, `string`, `array`, `dict` e `any`.
