/* kernel/ai/ai_core.h — Unified Intelligence Core */
#ifndef LUNA_AI_CORE_H
#define LUNA_AI_CORE_H
#include "../../include/types.h"

void        ai_core_init(void);
const char* ai_core_query(const char* prompt, uint8_t query_class);

#endif /* LUNA_AI_CORE_H */