# Simple Compiler/Interpreter — SCS 2312

A single-file C compiler/interpreter that tokenizes, parses, and executes a simple integer language.

## How to Compile and Run

```bash
gcc -o parser parser.c
./parser input.txt
```

On Windows:
```
gcc -o parser.exe parser.c
parser.exe input.txt
```

---

## Context-Free Grammar (CFG)

```
Program     → Statement*
Statement   → Declaration | PrintStmt
Declaration → 'int' ID '=' Expression ';'
PrintStmt   → 'print' '(' Expression ')' ';'
Expression  → Term ('+' Term)*
Term        → NUMBER | ID | '(' Expression ')'
```

- **Program** is zero or more statements.
- A **Statement** is either a variable declaration or a print statement.
- A **Declaration** starts with `int`, then a variable name, `=`, an expression, and `;`.
- A **PrintStmt** starts with `print`, then an expression inside parentheses, and `;`.
- An **Expression** is one or more terms connected by `+`.
- A **Term** is a number, a variable name, or a parenthesized expression.

---

## Tokenizer Design

The tokenizer (`tokenize()` function) reads the source code character by character and groups characters into **tokens**.

| Token Type       | Examples       | How it's recognized                    |
|------------------|----------------|----------------------------------------|
| `TOKEN_INT`      | `int`          | Word that matches "int"                |
| `TOKEN_PRINT`    | `print`        | Word that matches "print"              |
| `TOKEN_ID`       | `x`, `myVar`   | Word starting with a letter, not a keyword |
| `TOKEN_NUMBER`   | `5`, `100`     | Sequence of digits                     |
| `TOKEN_ASSIGN`   | `=`            | Single character                       |
| `TOKEN_PLUS`     | `+`            | Single character                       |
| `TOKEN_SEMICOLON`| `;`            | Single character                       |
| `TOKEN_LPAREN`   | `(`            | Single character                       |
| `TOKEN_RPAREN`   | `)`            | Single character                       |
| `TOKEN_EOF`      | —              | Added at end of token stream           |

**Algorithm**: Skip whitespace → if letter, read word → if digit, read number → otherwise match single character.

---

## Parser Functions

Each parser function corresponds to one CFG rule. The parser uses **recursive descent** — each rule is a function that calls other rule functions.

| Function             | CFG Rule                               | What it does                                      |
|----------------------|----------------------------------------|---------------------------------------------------|
| `parseProgram()`     | `Program → Statement*`                 | Loops parsing statements until EOF                |
| `parseStatement()`   | `Statement → Declaration \| PrintStmt` | Checks first token to decide which type           |
| `parseDeclaration()` | `Declaration → int ID = Expr ;`        | Reads name, evaluates expression, stores in table |
| `parsePrint()`       | `PrintStmt → print ( Expr ) ;`        | Evaluates expression and prints the result        |
| `parseExpression()`  | `Expression → Term (+ Term)*`          | Parses addition of terms                          |
| `parseTerm()`        | `Term → NUMBER \| ID \| ( Expr )`      | Returns value of number, variable, or sub-expr    |

**Helper functions**: `peek()` looks at current token, `advance()` consumes it, `expect()` checks and consumes or reports error.

---

## Example Input (`input.txt`)

```
int y = 5;
int x = 20;
int z = x + y;
print(z);
```

## Example Output

```
25
```

---

## Syntax Error Example (`syntax_error.txt`)

```
int x = ;
```

**Output:**
```
Error: Syntax error: Expected a number, variable, or '('
```

The parser expected a number or variable after `=` but found `;` instead.

---

## Semantic Error Example (`semantic_error.txt`)

```
print(a);
```

**Output:**
```
Error: Semantic error: Variable 'a' is not declared
```

The variable `a` was used without being declared with `int` first.

---

## Symbol Table

The symbol table is a simple array of structs. Each entry has:
- `name` — the variable name (string)
- `value` — the integer value

When a variable is declared (`int x = 5;`), an entry is added.
When a variable is used (`print(x);`), the table is searched using linear scan.

---

## Error Handling

| Error Type    | When it happens                         | Example                |
|---------------|----------------------------------------|------------------------|
| Syntax Error  | Token doesn't match what grammar expects| `int x = ;` (missing value) |
| Semantic Error| Variable used but never declared        | `print(a);` (a not declared) |
| Semantic Error| Variable declared twice                 | `int x = 5; int x = 10;`    |
