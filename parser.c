/*
 * parser.c - Simple Compiler/Interpreter
 * SCS 2312 - Computational Models and Programming Language Concepts
 *
 * This program reads a source file, tokenizes it, parses it using
 * recursive descent parsing, and executes simple integer programs.
 *
 * Supported language features:
 *   - Integer variable declarations:  int x = 5;
 *   - Addition expressions:           int z = x + y;
 *   - Print statements:               print(z);
 *
 * Context-Free Grammar (CFG):
 *   Program     -> Statement*
 *   Statement   -> Declaration | PrintStmt
 *   Declaration -> 'int' ID '=' Expression ';'
 *   PrintStmt   -> 'print' '(' Expression ')' ';'
 *   Expression  -> Term ('+' Term)*
 *   Term        -> NUMBER | ID | '(' Expression ')'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ===================== CONSTANTS ===================== */

#define MAX_TOKENS    500    /* maximum number of tokens */
#define MAX_TOKEN_LEN 100    /* maximum length of a single token */
#define MAX_SYMBOLS   100    /* maximum entries in symbol table */
#define MAX_SOURCE    10000  /* maximum source file size */

/* ===================== TOKEN TYPES ===================== */

/* Each token has a type so the parser knows what it is */
typedef enum {
    TOKEN_INT,        /* keyword: int */
    TOKEN_PRINT,      /* keyword: print */
    TOKEN_ID,         /* identifier (variable name) */
    TOKEN_NUMBER,     /* integer literal like 5, 20 */
    TOKEN_ASSIGN,     /* = */
    TOKEN_PLUS,       /* + */
    TOKEN_SEMICOLON,  /* ; */
    TOKEN_LPAREN,     /* ( */
    TOKEN_RPAREN,     /* ) */
    TOKEN_EOF         /* end of file/input */
} TokenType;

/* ===================== TOKEN STRUCTURE ===================== */

/* A token stores its type and its actual text value */
typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LEN];
} Token;

/* ===================== SYMBOL TABLE ENTRY ===================== */

/* Each variable gets an entry with its name and integer value */
typedef struct {
    char name[MAX_TOKEN_LEN];
    int value;
} Symbol;

/* ===================== GLOBAL VARIABLES ===================== */

Token tokens[MAX_TOKENS];  /* array of all tokens */
int tokenCount = 0;        /* how many tokens we found */
int currentToken = 0;      /* index of token we are currently looking at */

Symbol symbolTable[MAX_SYMBOLS];  /* array for storing variables */
int symbolCount = 0;              /* how many variables are stored */

/* ===================== ERROR HANDLING ===================== */

/*
 * error() - Prints an error message and stops the program.
 * This is used for both syntax errors and semantic errors.
 */
void error(const char *message) {
    printf("Error: %s\n", message);
    exit(1);
}

/* ===================== SYMBOL TABLE FUNCTIONS ===================== */

/*
 * lookupSymbol() - Searches for a variable by name in the symbol table.
 * Returns the index if found, or -1 if not found.
 * Uses simple linear search (no hash tables needed).
 */
int lookupSymbol(const char *name) {
    int i;
    for (i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return i;  /* found it */
        }
    }
    return -1;  /* not found */
}

/*
 * addSymbol() - Adds a new variable to the symbol table.
 * Stores the variable name and its integer value.
 */
void addSymbol(const char *name, int value) {
    if (symbolCount >= MAX_SYMBOLS) {
        error("Symbol table is full - too many variables");
    }
    strcpy(symbolTable[symbolCount].name, name);
    symbolTable[symbolCount].value = value;
    symbolCount++;
}

/* ===================== TOKENIZER ===================== */

/*
 * tokenize() - Breaks the source code into tokens.
 *
 * How it works:
 *   1. Read through the source code character by character
 *   2. Skip whitespace and newlines
 *   3. If we see a letter, read the whole word (could be keyword or identifier)
 *   4. If we see a digit, read the whole number
 *   5. If we see a special character (=, +, ;, etc.), make it a token
 *   6. Store each token in the tokens[] array
 */
void tokenize(const char *source) {
    int i = 0;
    int len = strlen(source);

    while (i < len) {
        /* Skip whitespace (spaces, tabs, newlines) */
        if (isspace(source[i])) {
            i++;
            continue;
        }

        /* Handle words: keywords (int, print) or identifiers (variable names) */
        if (isalpha(source[i]) || source[i] == '_') {
            int start = i;
            char word[MAX_TOKEN_LEN];
            int wordLen = 0;

            /* Read the entire word */
            while (i < len && (isalnum(source[i]) || source[i] == '_')) {
                word[wordLen++] = source[i++];
            }
            word[wordLen] = '\0';

            /* Check if it's a keyword or an identifier */
            if (strcmp(word, "int") == 0) {
                tokens[tokenCount].type = TOKEN_INT;
            } else if (strcmp(word, "print") == 0) {
                tokens[tokenCount].type = TOKEN_PRINT;
            } else {
                tokens[tokenCount].type = TOKEN_ID;
            }
            strcpy(tokens[tokenCount].value, word);
            tokenCount++;
            continue;
        }

        /* Handle numbers: read all consecutive digits */
        if (isdigit(source[i])) {
            char number[MAX_TOKEN_LEN];
            int numLen = 0;

            while (i < len && isdigit(source[i])) {
                number[numLen++] = source[i++];
            }
            number[numLen] = '\0';

            tokens[tokenCount].type = TOKEN_NUMBER;
            strcpy(tokens[tokenCount].value, number);
            tokenCount++;
            continue;
        }

        /* Handle single-character tokens */
        switch (source[i]) {
            case '=':
                tokens[tokenCount].type = TOKEN_ASSIGN;
                strcpy(tokens[tokenCount].value, "=");
                tokenCount++;
                break;
            case '+':
                tokens[tokenCount].type = TOKEN_PLUS;
                strcpy(tokens[tokenCount].value, "+");
                tokenCount++;
                break;
            case ';':
                tokens[tokenCount].type = TOKEN_SEMICOLON;
                strcpy(tokens[tokenCount].value, ";");
                tokenCount++;
                break;
            case '(':
                tokens[tokenCount].type = TOKEN_LPAREN;
                strcpy(tokens[tokenCount].value, "(");
                tokenCount++;
                break;
            case ')':
                tokens[tokenCount].type = TOKEN_RPAREN;
                strcpy(tokens[tokenCount].value, ")");
                tokenCount++;
                break;
            default:
                printf("Error: Unknown character '%c'\n", source[i]);
                exit(1);
        }
        i++;
    }

    /* Add EOF token to mark the end */
    tokens[tokenCount].type = TOKEN_EOF;
    strcpy(tokens[tokenCount].value, "EOF");
    tokenCount++;
}

/* ===================== PARSER HELPER FUNCTIONS ===================== */

/*
 * peek() - Returns the current token without consuming it.
 * This lets us look ahead to decide what to parse next.
 */
Token peek(void) {
    return tokens[currentToken];
}

/*
 * advance() - Returns the current token and moves to the next one.
 * This "consumes" the token.
 */
Token advance(void) {
    return tokens[currentToken++];
}

/*
 * expect() - Checks that the current token is the expected type.
 * If it matches, consume it. If not, report a syntax error.
 * This is how we enforce the grammar rules.
 */
Token expect(TokenType type, const char *errorMsg) {
    Token t = peek();
    if (t.type != type) {
        error(errorMsg);
    }
    return advance();
}

/* ===================== PARSER FUNCTIONS ===================== */
/*
 * These functions implement recursive descent parsing.
 * Each function corresponds to one rule in the CFG.
 *
 * The parser also EXECUTES the code as it parses (interpreter style).
 * When it sees a declaration, it evaluates the expression and stores
 * the result. When it sees print, it evaluates and prints the result.
 */

/* Forward declarations (needed because functions call each other) */
int parseExpression(void);
int parseTerm(void);

/*
 * parseTerm() - Parses a single term in an expression.
 *
 * CFG Rule: Term -> NUMBER | ID | '(' Expression ')'
 *
 * A term can be:
 *   - A number like 5 or 20
 *   - A variable name like x or y
 *   - A parenthesized expression like (x + 5)
 */
int parseTerm(void) {
    Token t = peek();

    /* Case 1: It's a number - convert to integer and return */
    if (t.type == TOKEN_NUMBER) {
        advance();
        return atoi(t.value);
    }

    /* Case 2: It's an identifier - look up its value in symbol table */
    if (t.type == TOKEN_ID) {
        advance();
        int index = lookupSymbol(t.value);
        if (index == -1) {
            /* SEMANTIC ERROR: variable was never declared */
            char msg[200];
            sprintf(msg, "Semantic error: Variable '%s' is not declared", t.value);
            error(msg);
        }
        return symbolTable[index].value;
    }

    /* Case 3: It's a parenthesized expression */
    if (t.type == TOKEN_LPAREN) {
        advance();  /* consume '(' */
        int result = parseExpression();
        expect(TOKEN_RPAREN, "Syntax error: Expected ')' after expression");
        return result;
    }

    /* If none of the above, it's a syntax error */
    error("Syntax error: Expected a number, variable, or '('");
    return 0;  /* never reached, but keeps compiler happy */
}

/*
 * parseExpression() - Parses an expression with addition.
 *
 * CFG Rule: Expression -> Term ('+' Term)*
 *
 * Examples:
 *   5           (just a term)
 *   x + y       (two terms added)
 *   x + y + 10  (three terms added)
 */
int parseExpression(void) {
    int result = parseTerm();  /* parse the first term */

    /* Keep adding terms while we see '+' */
    while (peek().type == TOKEN_PLUS) {
        advance();  /* consume '+' */
        int right = parseTerm();
        result = result + right;
    }

    return result;
}

/*
 * parseDeclaration() - Parses a variable declaration.
 *
 * CFG Rule: Declaration -> 'int' ID '=' Expression ';'
 *
 * Example: int x = 5 + 3;
 *   1. We already consumed 'int' before calling this
 *   2. Read the variable name (ID)
 *   3. Expect '='
 *   4. Evaluate the expression
 *   5. Expect ';'
 *   6. Store variable in symbol table
 */
void parseDeclaration(void) {
    /* Get the variable name */
    Token nameToken = expect(TOKEN_ID, "Syntax error: Expected variable name after 'int'");

    /* Check if variable already exists */
    if (lookupSymbol(nameToken.value) != -1) {
        char msg[200];
        sprintf(msg, "Semantic error: Variable '%s' is already declared", nameToken.value);
        error(msg);
    }

    /* Expect '=' sign */
    expect(TOKEN_ASSIGN, "Syntax error: Expected '=' after variable name");

    /* Evaluate the expression on the right side */
    int value = parseExpression();

    /* Expect semicolon at the end */
    expect(TOKEN_SEMICOLON, "Syntax error: Expected ';' at end of declaration");

    /* Store the variable and its value in the symbol table */
    addSymbol(nameToken.value, value);
}

/*
 * parsePrint() - Parses a print statement.
 *
 * CFG Rule: PrintStmt -> 'print' '(' Expression ')' ';'
 *
 * Example: print(z);
 *   1. We already consumed 'print' before calling this
 *   2. Expect '('
 *   3. Evaluate the expression inside
 *   4. Expect ')'
 *   5. Expect ';'
 *   6. Print the result
 */
void parsePrint(void) {
    /* Expect opening parenthesis */
    expect(TOKEN_LPAREN, "Syntax error: Expected '(' after 'print'");

    /* Evaluate the expression inside the parentheses */
    int value = parseExpression();

    /* Expect closing parenthesis */
    expect(TOKEN_RPAREN, "Syntax error: Expected ')' after expression in print");

    /* Expect semicolon */
    expect(TOKEN_SEMICOLON, "Syntax error: Expected ';' after print statement");

    /* Print the result */
    printf("%d\n", value);
}

/*
 * parseStatement() - Parses a single statement.
 *
 * CFG Rule: Statement -> Declaration | PrintStmt
 *
 * Looks at the current token to decide what kind of statement it is:
 *   - If it starts with 'int', it's a declaration
 *   - If it starts with 'print', it's a print statement
 *   - Anything else is an error
 */
void parseStatement(void) {
    Token t = peek();

    if (t.type == TOKEN_INT) {
        advance();  /* consume 'int' */
        parseDeclaration();
    } else if (t.type == TOKEN_PRINT) {
        advance();  /* consume 'print' */
        parsePrint();
    } else {
        error("Syntax error: Expected 'int' or 'print' at start of statement");
    }
}

/*
 * parseProgram() - Parses the entire program.
 *
 * CFG Rule: Program -> Statement*
 *
 * Keeps parsing statements until we reach the end of the file.
 */
void parseProgram(void) {
    while (peek().type != TOKEN_EOF) {
        parseStatement();
    }
}

/* ===================== MAIN FUNCTION ===================== */

int main(int argc, char *argv[]) {
    FILE *file;
    char source[MAX_SOURCE];
    int fileSize;

    /* Check if user provided a file name */
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        printf("Example: %s input.txt\n", argv[0]);
        return 1;
    }

    /* Open the source file */
    file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }

    /* Read the entire file into the source buffer */
    fileSize = fread(source, 1, MAX_SOURCE - 1, file);
    source[fileSize] = '\0';
    fclose(file);

    /* Step 1: Tokenize - break source code into tokens */
    tokenize(source);

    /* Step 2: Parse and Execute - parse the token stream and run the program */
    parseProgram();

    return 0;
}
