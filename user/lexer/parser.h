/* user/lexer/parser.h — Abstract Syntax Tree Builder */
#ifndef LUNA_PARSER_H
#define LUNA_PARSER_H

#include "token.h"
#include "../../kernel/arena.h"

typedef enum {
    AST_PROGRAM,
    AST_VAR_DECL,
    AST_FUNC_DECL,
    AST_BINARY_OP,
    AST_LITERAL
} AST_NodeType;

typedef struct AST_Node {
    AST_NodeType type;
    struct AST_Node* left;
    struct AST_Node* right;
    Token token;
} AST_Node;

AST_Node* parser_parse(const char* source);

#endif /* LUNA_PARSER_H */