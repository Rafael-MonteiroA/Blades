# Blades — Grammar Specification

> **Status:** Placeholder — will be populated during Phase 2 (Parser & AST).

This document will contain the formal grammar of the Blades language using
Extended Backus-Naur Form (EBNF) notation.

---

## Notation

```
rule        =  definition ;
definition  =  alternatives ;
alternatives = sequence { '|' sequence } ;
sequence    =  term { term } ;
term        =  literal | rule-name | '[' alternatives ']'   (* optional *)
             | '{' alternatives '}'                         (* zero or more *)
             | '(' alternatives ')' ;                       (* grouping *)
```

---

## Placeholder (Phase 2)

Grammar will be defined here once the parser is implemented in Phase 2.
Sections will cover:

- **Lexical grammar** (tokens: identifiers, literals, keywords, operators)
- **Expression grammar** (precedence levels, associativity)
- **Statement grammar** (let, mut, if, while, for, return, match)
- **Declaration grammar** (fn, struct, impl, trait, use)
- **Top-level grammar** (module structure)

---

*Last updated: Phase 0 — Engineering Foundation (placeholder)*
