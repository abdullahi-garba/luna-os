/* user/lexer/lexer.h — Luna Script Lexer & CLI Intent Identifier */
#ifndef LUNA_LEXER_H
#define LUNA_LEXER_H

#include "../../include/types.h"
#include "token.h"

/* ── Scripting Language Lexer ────────────────────────────────────────────── */
void  lexer_init(const char* source);
Token lexer_next_token(void);

/* ── CLI NLP Intent Mapper ───────────────────────────────────────────────── */
typedef enum {
    INTENT_UNKNOWN,
    INTENT_LIST_FILES,
    INTENT_READ_FILE,
    INTENT_WRITE_FILE,
    INTENT_DELETE_FILE,
    INTENT_PROCESS_LIST,
    INTENT_SYSTEM_HALT,
    INTENT_AI_QUERY
} CLI_Intent;

CLI_Intent lexer_identify_intent(const char* english_command);

#endif /* LUNA_LEXER_H */