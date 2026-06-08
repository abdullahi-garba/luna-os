/* kernel/ai/ai_core.c — Luna AI Core
 *
 * Unified dispatch layer combining:
 *   1. Explicit Engine  — deterministic rule-based simulation AI
 *                         (like RollerCoaster Tycoon 1999's agent system)
 *   2. Neural Core      — transformer-based LLM inference (quantized, no_std)
 *
 * Fusion strategy:
 *   - Explicit Engine always runs first (it is ground truth for the OS state)
 *   - Neural Core runs for language understanding / generative responses
 *   - Fusion layer weights: Explicit=0.7, Neural=0.3 for factual queries
 *                           Explicit=0.2, Neural=0.8 for language generation
 *
 * Platform deployment:
 *   - Desktop/Mobile: Both engines active
 *   - IoT: Explicit Engine only (LUNA_AI_IOT_MODE defined in build)
 */

#include "ai_core.h"
#include "explicit/explicit.h"
#include "nlp/nlp.h"
#include "../../kernel/arena.h"
#include "../../kernel/string.h"
#include "../../include/types.h"

#ifndef LUNA_AI_IOT_MODE
#include "neural/neural.h"
#endif

/* ── AI query result pool ────────────────────────────────────────────────── */
#define AI_RESPONSE_BUF_SIZE 2048

static char ai_response_buf[AI_RESPONSE_BUF_SIZE];

/* ── Confidence fusion ───────────────────────────────────────────────────── */
typedef struct {
    char     text[AI_RESPONSE_BUF_SIZE];
    uint32_t confidence_pct;  /* 0–100 */
    uint8_t  source;          /* AI_SOURCE_EXPLICIT | AI_SOURCE_NEURAL | AI_SOURCE_FUSED */
} AIPartialResult;

/* ── Query classification ────────────────────────────────────────────────── */
static QueryClass classify_query(const char* input) {
    /* Route based on keyword presence — fast static trie (no malloc) */
    if (luna_strstr(input, "simulate") || luna_strstr(input, "agent") ||
        luna_strstr(input, "world") || luna_strstr(input, "constraint")) {
        return QUERY_SIMULATION;
    }
    if (luna_strstr(input, "net") || luna_strstr(input, "scan") ||
        luna_strstr(input, "port") || luna_strstr(input, "packet")) {
        return QUERY_SECURITY;
    }
    if (luna_strstr(input, "file") || luna_strstr(input, "disk") ||
        luna_strstr(input, "forensic") || luna_strstr(input, "hash")) {
        return QUERY_FORENSIC;
    }
    if (luna_strstr(input, "explain") || luna_strstr(input, "what is") ||
        luna_strstr(input, "how do") || luna_strstr(input, "tell me")) {
        return QUERY_LANGUAGE;
    }
    return QUERY_GENERAL;
}

/* ── Init ────────────────────────────────────────────────────────────────── */
void ai_core_init(void) {
    explicit_init();
    nlp_init();

#ifndef LUNA_AI_IOT_MODE
    neural_init();
#endif
}

/* ── Primary query entry point ───────────────────────────────────────────── */
const char* ai_query(const char* input, AIQueryContext* ctx) {
    if (!input || !*input) return "[AI] Empty query";

    QueryClass qclass = classify_query(input);
    AIPartialResult explicit_result = {0};
    AIPartialResult neural_result   = {0};

    /* ── Explicit Engine ────────────────────────────────────────────────── */
    {
        ExplicitQuery eq;
        luna_strncpy(eq.input, input, sizeof(eq.input) - 1);
        eq.query_class = qclass;
        eq.context_ptr = ctx;

        ExplicitResult er = explicit_evaluate(&eq);
        luna_strncpy(explicit_result.text, er.response, AI_RESPONSE_BUF_SIZE - 1);
        explicit_result.confidence_pct = er.confidence_pct;
        explicit_result.source = AI_SOURCE_EXPLICIT;
    }

#ifndef LUNA_AI_IOT_MODE
    /* ── Neural Core (Desktop/Mobile only) ─────────────────────────────── */
    {
        NeuralQuery nq;
        luna_strncpy(nq.prompt, input, sizeof(nq.prompt) - 1);
        nq.max_tokens  = 256;
        nq.temperature = 70; /* 0.70 scaled to int: temperature/100.0 */
        nq.context_ptr = ctx;

        NeuralResult nr = neural_infer(&nq);
        luna_strncpy(neural_result.text, nr.response, AI_RESPONSE_BUF_SIZE - 1);
        neural_result.confidence_pct = nr.confidence_pct;
        neural_result.source = AI_SOURCE_NEURAL;
    }

    /* ── Fusion layer ───────────────────────────────────────────────────── */
    /* Weight matrix by query class */
    uint32_t explicit_weight, neural_weight;

    switch (qclass) {
    case QUERY_SIMULATION:
    case QUERY_SECURITY:
    case QUERY_FORENSIC:
        explicit_weight = 75;
        neural_weight   = 25;
        break;
    case QUERY_LANGUAGE:
    case QUERY_GENERAL:
    default:
        explicit_weight = 25;
        neural_weight   = 75;
        break;
    }

    /* Weighted confidence selection */
    uint32_t explicit_score = (explicit_result.confidence_pct * explicit_weight) / 100;
    uint32_t neural_score   = (neural_result.confidence_pct   * neural_weight)   / 100;

    if (explicit_score >= neural_score) {
        /* Explicit result is authoritative — prepend neural context if useful */
        if (neural_result.confidence_pct > 60 &&
            qclass == QUERY_LANGUAGE) {
            /* Compose: use neural response with explicit verification note */
            luna_snprintf(ai_response_buf, AI_RESPONSE_BUF_SIZE,
                "[Verified] %s\n[Context] %s",
                explicit_result.text, neural_result.text);
        } else {
            luna_strncpy(ai_response_buf, explicit_result.text, AI_RESPONSE_BUF_SIZE - 1);
        }
    } else {
        /* Neural result wins — annotate if explicit had a constraint violation */
        if (explicit_result.confidence_pct == 0) {
            luna_snprintf(ai_response_buf, AI_RESPONSE_BUF_SIZE,
                "[CONSTRAINT BLOCKED] %s", neural_result.text);
        } else {
            luna_strncpy(ai_response_buf, neural_result.text, AI_RESPONSE_BUF_SIZE - 1);
        }
    }

#else
    /* IoT mode — Explicit Engine only */
    luna_strncpy(ai_response_buf, explicit_result.text, AI_RESPONSE_BUF_SIZE - 1);
#endif

    ai_response_buf[AI_RESPONSE_BUF_SIZE - 1] = '\0';
    return ai_response_buf;
}

/* ── Shorthand: process a shell command through AI for suggestions ────────── */
const char* ai_suggest_command(const char* english_input) {
    AIQueryContext ctx;
    ctx.caller_ring = 0;
    ctx.restrict_to_shell = true;
    return ai_query(english_input, &ctx);
}
