
// SCS 2312 - Computational Models and Programming Language Concepts
// Assignment
// 23000066 - Amarasekara A R M K T

// parser.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS    500    //max tokens
#define MAX_TOKEN_LEN 100    // single token max length
#define MAX_SYMBOLS   100    //max entries in symbol table
#define MAX_SOURCE    10000  // maximum input file size

// tokens list
typedef enum {
    INT,        // keyword: int
    PRINT,      // keyword: print
    ID,      // identifier: variable name
    NUMBER,     // integer: 5, 20
    ASSIGN,     // =
    PLUS,       // +
    SEMICOLON,  // ;
    LPAREN,     // (
    RPAREN,     // )
    EOF         // end of input file
} TokenType;

// token structure

// each token stores its type and its actual text value
typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LEN];
} Token;

// symbol table entry

// each variable gets an entry with its name and integer value
typedef struct {
    char name[MAX_TOKEN_LEN];
    int value;
} Symbol;



Token tokens[MAX_TOKENS];  // all tokens array
int tokenCount = 0;        // tokens count found
int currentToken = 0;      // index of token htat currently looking at

Symbol symbolTable[MAX_SYMBOLS];  // variables array
int symbolCount = 0;              // no.of variables stored



// error handling

// prints error and stop
// both syntax and semantic errors
void error(const char *message) {
    printf("Error: %s\n", message);
    exit(1);
}

// symbol table functions

// Search variables by name in the symbol table.
// Returns the index if found, -1 if not found.
// linear search
int lookupSymbol(const char *name) {
    int i;
    for (i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Add new variables to the symbol table
// Store variable name and its integer value.
void addSymbol(const char *name, int value) {
    if (symbolCount >= MAX_SYMBOLS) {
        error("Symbol table is full - too many variables");
    }
    strcpy(symbolTable[symbolCount].name, name);
    symbolTable[symbolCount].value = value;
    symbolCount++;
}

//TOKENIZER
void tokenize(const char *source) {
    int i = 0;
    int len = strlen(source);

    while (i < len) {
        // Skip whitespace (spaces, tabs, newlines)
        if (isspace(source[i])) {
            i++;
            continue;
        }

        //keywords (int, print) or identifiers (variables)
        if (isalpha(source[i]) || source[i] == '_') {
            int start = i;
            char word[MAX_TOKEN_LEN];
            int wordLen = 0;

            // Read the entire word
            while (i < len && (isalnum(source[i]) || source[i] == '_')) {
                word[wordLen++] = source[i++];
            }
            word[wordLen] = '\0';

            // Check if keyword or identifier
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

        // handle numbers
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

        // handle special characters
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

    // Add EOF token to mark the end of input
    tokens[tokenCount].type = TOKEN_EOF;
    strcpy(tokens[tokenCount].value, "EOF");
    tokenCount++;
}

// PARSER HELPER FUNCTIONS

// returns the current token without consuming it.
// looks ahead to decide what to parse next.
Token peek(void) {
    return tokens[currentToken];
}

// returns the current token and moves to the next one.
// consumes the token.
Token advance(void) {
    return tokens[currentToken++];
}

//checks that the current token is the expected type.
//If it matches, consume it. If not, report syntax error.
//handle grammar rules.
Token expect(TokenType type, const char *errorMsg) {
    Token t = peek();
    if (t.type != type) {
        error(errorMsg);
    }
    return advance();
}

// PARSER FUNCTIONS

// pre declarations because functions call each other
int parseExpression(void);
int parseTerm(void);

// Parses a single term in an expression.
// CFG Rule: Term > NUMBER | ID | '(' Expression ')'
int parseTerm(void) {
    Token t = peek();

    // Case 1: number > convert to integer and return
    if (t.type == TOKEN_NUMBER) {
        advance();
        return atoi(t.value);
    }

    // Case 2: identifier > check if its value in symbol table
    if (t.type == TOKEN_ID) {
        advance();
        int index = lookupSymbol(t.value);
        if (index == -1) {
            // SEMANTIC ERROR: variable was never declared
            char msg[200];
            sprintf(msg, "Semantic error: Variable '%s' is not declared", t.value);
            error(msg);
        }
        return symbolTable[index].value;
    }

    // Case 3: parenthesized expression
    if (t.type == TOKEN_LPAREN) {
        advance();  // consume '(' 
        int result = parseExpression();
        expect(TOKEN_RPAREN, "Syntax error: Expected ')' after expression");
        return result;
    }

    // If none of the above > syntax error
    error("Syntax error: Expected a number, variable, or '('");
    return 0;
}

//Parses an expression with addition.
// CFG Rule: Expression > Term ('+' Term)*
int parseExpression(void) {
    int result = parseTerm();  // parse the first term

    // do additon
    while (peek().type == TOKEN_PLUS) {
        advance();  // consume '+'
        int right = parseTerm();
        result = result + right;
    }

    return result;
}

// Parses a variable declaration.
// CFG Rule: Declaration > 'int' ID '=' Expression ';'
void parseDeclaration(void) {
    // Get the variable name
    Token nameToken = expect(TOKEN_ID, "Syntax error: Expected variable name after 'int'");

    // Check if variable already exists
    if (lookupSymbol(nameToken.value) != -1) {
        char msg[200];
        sprintf(msg, "Semantic error: Variable '%s' is already declared", nameToken.value);
        error(msg);
    }

    // Expect '=' sign
    expect(TOKEN_ASSIGN, "Syntax error: Expected '=' after variable name");

    // Evaluate the expression on the right side
    int value = parseExpression();

    // Expect semicolon at the end
    expect(TOKEN_SEMICOLON, "Syntax error: Expected ';' at end of declaration");

    // Store the variable and its value in the symbol table
    addSymbol(nameToken.value, value);
}

// parses a print statement.
// CFG Rule: PrintStmt > 'print' '(' Expression ')' ';'
void parsePrint(void) {
    // Expect opening parenthesis
    expect(TOKEN_LPAREN, "Syntax error: Expected '(' after 'print'");

    // Evaluate the expression inside the parentheses
    int value = parseExpression();

    // Expect closing parenthesis
    expect(TOKEN_RPAREN, "Syntax error: Expected ')' after expression in print");

    // Expect semicolon
    expect(TOKEN_SEMICOLON, "Syntax error: Expected ';' after print statement");

    printf("%d\n", value);
}

// Parses a single statement.
// CFG Rule: Statement > Declaration | PrintStmt
void parseStatement(void) {
    Token t = peek();

    if (t.type == TOKEN_INT) {
        advance();  
        parseDeclaration();
    } else if (t.type == TOKEN_PRINT) {
        advance();  
        parsePrint();
    } else {
        error("Syntax error: Expected 'int' or 'print' at start of statement");
    }
}

// parses the entire program.
// CFG Rule: Program > Statement*
void parseProgram(void) {
    while (peek().type != TOKEN_EOF) {
        parseStatement();
    }
}


int main(int argc, char *argv[]) {
    FILE *file;
    char source[MAX_SOURCE];
    int fileSize;

    // Check for file name
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        printf("Example: %s input.txt\n", argv[0]);
        return 1;
    }

    // Open the source file
    file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }

    // push the entire file into the source buffer
    fileSize = fread(source, 1, MAX_SOURCE - 1, file);
    source[fileSize] = '\0';
    fclose(file);

    // Tokenize
    tokenize(source);

    // Parse and Execute
    parseProgram();

    return 0;
}
