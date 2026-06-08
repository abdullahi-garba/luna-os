/* user/lexer/lexer.c — Tokenizer and Intent Engine */
#include "lexer.h"
#include "../../kernel/string.h"

static const char* src;
static int pos = 0;
static int line = 1;
static int col = 1;

void lexer_init(const char* source) {
    src = source;
    pos = 0;
    line = 1;
    col = 1;
}

Token lexer_next_token(void) {
    Token tok;
    luna_memset(&tok, 0, sizeof(Token));
    tok.type = TOK_EOF;
    
    /* Consume whitespace */
    while (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r') {
        if (src[pos] == '\n') { line++; col = 1; }
        else { col++; }
        pos++;
    }

    if (src[pos] == '\0') return tok;

    tok.line = line;
    tok.col = col;

    /* TODO: Full hybrid token extraction logic (let, fn, =>, indent tracking) */
    tok.type = TOK_IDENT;
    tok.as_str[0] = src[pos];
    tok.as_str[1] = '\0';
    pos++; col++;
    
    return tok;
}

/* ── Natural Language to System Command Router ───────────────────────────── */
CLI_Intent lexer_identify_intent(const char* cmd) {
    /* Fast-path string matching to route English to system binaries */
    if (luna_strstr(cmd, "list") || luna_strstr(cmd, "show files") || luna_strstr(cmd, "dir")) 
        return INTENT_LIST_FILES;
    if (luna_strstr(cmd, "read") || luna_strstr(cmd, "show me the content") || luna_strstr(cmd, "cat")) 
        return INTENT_READ_FILE;
    if (luna_strstr(cmd, "delete") || luna_strstr(cmd, "remove") || luna_strstr(cmd, "destroy")) 
        return INTENT_DELETE_FILE;
    if (luna_strstr(cmd, "process") || luna_strstr(cmd, "tasks") || luna_strstr(cmd, "running")) 
        return INTENT_PROCESS_LIST;
    if (luna_strstr(cmd, "shut down") || luna_strstr(cmd, "halt") || luna_strstr(cmd, "turn off")) 
        return INTENT_SYSTEM_HALT;
    if (luna_strstr(cmd, "ask ai") || luna_strstr(cmd, "query") || luna_strstr(cmd, "help me understand")) 
        return INTENT_AI_QUERY;
        
    return INTENT_UNKNOWN;
}