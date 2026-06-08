/* user/lexer/parser.c — Scripting Language Parser
 * Constructs the AST entirely within the transient lang_arena.
 * This guarantees zero memory fragmentation during script execution.
 */
#include "parser.h"
#include "lexer.h"
#include "../../kernel/string.h"

static AST_Node* create_node(AST_NodeType type) {
    /* O(1) transient allocation. No standard malloc/free. */
    AST_Node* node = (AST_Node*)arena_alloc(&g_lang_arena, sizeof(AST_Node));
    if (node) {
        luna_memset(node, 0, sizeof(AST_Node));
        node->type = type;
    }
    return node;
}

AST_Node* parser_parse(const char* source) {
    /* 1. Reset arena before parsing new script to clear old executions */
    arena_reset(&g_lang_arena);
    
    lexer_init(source);
    
    /* 2. Build root node */
    AST_Node* root = create_node(AST_PROGRAM);
    if (!root) return NULL;
    
    /* 3. Recursive descent parsing loop */
    Token t = lexer_next_token();
    while (t.type != TOK_EOF && t.type != TOK_ERROR) {
        /* TODO: Map tokens to AST_VAR_DECL, AST_FUNC_DECL, etc. */
        t = lexer_next_token();
    }
    
    return root;
}