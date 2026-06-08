/* ai/explicit/explicit.h — Deterministic OS State Model */
#ifndef LUNA_EXPLICIT_AI_H
#define LUNA_EXPLICIT_AI_H

#include "../../include/types.h"

typedef enum {
    QUERY_SYSTEM_STATE,
    QUERY_SECURITY_AUDIT,
    QUERY_LANGUAGE
} ExplicitQueryClass;

typedef struct {
    char     text[1024];
    uint32_t confidence_pct;
} ExplicitResult;

void explicit_ai_init(void);
ExplicitResult explicit_ai_query(const char* query, ExplicitQueryClass qclass);

#endif /* LUNA_EXPLICIT_AI_H */