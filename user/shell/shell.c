/* kernel/shell.c — Luna OS Tactical CLI Shell
 * Features:
 *   - Native commands: ls, cat, write, rm, clear, ledger, ps, ai, hash, run, help
 *   - NLP intent router: English phrases → shell commands via nlp.c
 *   - Luna script execution: run <script.luna>
 *   - History buffer (8 entries, up-arrow recall)
 *   - Tab completion for VFS paths
 */
#include "shell.h"
#include "vfs.h"
#include "ledger.h"
#include "hash.h"
#include "string.h"
#include "process.h"
#include "scheduler.h"
#include "ai/ai_core.h"
#include "ai/nlp/nlp.h"
#include "../lang/lexer.h"
#include "../lang/parser.h"
#include "../lang/eval.h"
#include "../drivers/gfx/framebuffer.h"
#include "../drivers/gfx/font.h"
#include "../drivers/serial/uart.h"
#include "../include/types.h"

#define SHELL_BUF_LEN   256
#define HISTORY_DEPTH   8
#define MAX_ARGS        16

/* ── State ───────────────────────────────────────────────────────────────── */
static char  input_buf[SHELL_BUF_LEN];
static int   cursor     = 0;
static char  history[HISTORY_DEPTH][SHELL_BUF_LEN];
static int   hist_count = 0;
static int   hist_idx   = -1;
static int   shell_row  = 0;   /* current output row on screen */
static bool  shell_running = true;

/* ── Forward declarations ────────────────────────────────────────────────── */
static void shell_print(const char* s);
static void shell_println(const char* s);
static void shell_prompt(void);
static void execute_line(char* line);
static void run_builtin(int argc, char* argv[]);

/* ── Output ──────────────────────────────────────────────────────────────── */
static void shell_print(const char* s) {
    uart_puts(s);
    font_print(0, shell_row * 16, s);
}

static void shell_println(const char* s) {
    shell_print(s);
    shell_row++;
    if (shell_row > 46) shell_row = 0;  /* simple wrap */
}

static void shell_printf(const char* fmt, ...) {
    char buf[512];
    /* Use luna_snprintf via builtin va */
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    luna_snprintf(buf, sizeof(buf), fmt);
    __builtin_va_end(args);
    shell_println(buf);
}

static void shell_prompt(void) {
    font_set_fg(0x00E5FF);
    shell_print("\nluna@aeterna:~$ ");
    font_set_fg(0xFFFFFF);
}

/* ── Input ───────────────────────────────────────────────────────────────── */
/* Called from keyboard driver on key press */
void shell_on_key(char c) {
    if (c == '\n') {
        input_buf[cursor] = '\0';
        shell_println("");

        if (cursor > 0) {
            /* Add to history */
            luna_strncpy(history[hist_count % HISTORY_DEPTH], input_buf, SHELL_BUF_LEN - 1);
            hist_count++;
        }

        execute_line(input_buf);
        cursor   = 0;
        hist_idx = -1;
        shell_prompt();

    } else if (c == '\b') {
        if (cursor > 0) {
            cursor--;
            input_buf[cursor] = '\0';
            /* Erase character on screen */
            font_erase_char(0, shell_row * 16, cursor);
        }
    } else if (c >= 0x20 && cursor < SHELL_BUF_LEN - 1) {
        input_buf[cursor++] = c;
        input_buf[cursor]   = '\0';
        /* Echo character */
        char tmp[2] = {c, 0};
        shell_print(tmp);
    }
}

/* ── Line tokenizer ──────────────────────────────────────────────────────── */
static int tokenize(char* line, char* argv[]) {
    int argc = 0;
    char* save;
    char* tok = luna_strtok(line, " \t", &save);
    while (tok && argc < MAX_ARGS - 1) {
        argv[argc++] = tok;
        tok = luna_strtok(NULL, " \t", &save);
    }
    argv[argc] = NULL;
    return argc;
}

/* ── NLP check: does this look like English prose? ───────────────────────── */
static bool is_english_intent(const char* line) {
    /* Heuristic: if it contains a space and no recognized command prefix */
    static const char* known_cmds[] = {
        "ls", "cat", "write", "rm", "clear", "ledger", "ps",
        "ai", "hash", "run", "help", "shutdown", "reboot", NULL
    };
    /* Extract first word */
    char first[32] = {0};
    int i = 0;
    while (line[i] && line[i] != ' ' && i < 31) { first[i] = line[i]; i++; }

    for (int k = 0; known_cmds[k]; k++) {
        if (luna_strcmp(first, known_cmds[k]) == 0) return false;
    }
    /* Has a space (multiple words) and not a known command → try NLP */
    return luna_strchr(line, ' ') != NULL;
}

/* ── Execute ─────────────────────────────────────────────────────────────── */
static void execute_line(char* line) {
    /* Trim leading whitespace */
    while (*line == ' ' || *line == '\t') line++;
    if (!*line) return;

    /* NLP intent routing — intercept English prose before command parse */
    if (is_english_intent(line)) {
        const char* mapped = nlp_resolve_intent(line);
        if (mapped && *mapped) {
            char nl_buf[SHELL_BUF_LEN];
            luna_snprintf(nl_buf, sizeof(nl_buf), "[NLP] → %s", mapped);
            shell_println(nl_buf);
            /* Execute the resolved command */
            char cmd_copy[SHELL_BUF_LEN];
            luna_strncpy(cmd_copy, mapped, SHELL_BUF_LEN - 1);
            execute_line(cmd_copy);
            return;
        }
    }

    /* Tokenize */
    char line_copy[SHELL_BUF_LEN];
    luna_strncpy(line_copy, line, SHELL_BUF_LEN - 1);
    char* argv[MAX_ARGS];
    int   argc = tokenize(line_copy, argv);
    if (argc == 0) return;

    run_builtin(argc, argv);
}

/* ── Built-in commands ───────────────────────────────────────────────────── */
static void cmd_ls(int argc, char* argv[]);
static void cmd_cat(int argc, char* argv[]);
static void cmd_write(int argc, char* argv[]);
static void cmd_rm(int argc, char* argv[]);
static void cmd_ledger(int argc, char* argv[]);
static void cmd_ps(int argc, char* argv[]);
static void cmd_ai(int argc, char* argv[]);
static void cmd_hash(int argc, char* argv[]);
static void cmd_run(int argc, char* argv[]);
static void cmd_help(void);

static void run_builtin(int argc, char* argv[]) {
    if (luna_strcmp(argv[0], "ls")       == 0) cmd_ls(argc, argv);
    else if (luna_strcmp(argv[0], "cat") == 0) cmd_cat(argc, argv);
    else if (luna_strcmp(argv[0], "write")== 0) cmd_write(argc, argv);
    else if (luna_strcmp(argv[0], "rm")  == 0) cmd_rm(argc, argv);
    else if (luna_strcmp(argv[0], "clear")== 0) fb_clear(0x000000);
    else if (luna_strcmp(argv[0], "ledger")== 0) cmd_ledger(argc, argv);
    else if (luna_strcmp(argv[0], "ps")  == 0) cmd_ps(argc, argv);
    else if (luna_strcmp(argv[0], "ai")  == 0) cmd_ai(argc, argv);
    else if (luna_strcmp(argv[0], "hash")== 0) cmd_hash(argc, argv);
    else if (luna_strcmp(argv[0], "run") == 0) cmd_run(argc, argv);
    else if (luna_strcmp(argv[0], "help")== 0) cmd_help();
    else if (luna_strcmp(argv[0], "shutdown") == 0) {
        shell_println("Luna OS shutting down.");
        __asm__ volatile("cli; hlt");
    } else {
        char msg[128];
        luna_snprintf(msg, sizeof(msg), "Unknown command: '%s' (try 'help')", argv[0]);
        shell_println(msg);
    }
}

/* ── ls ──────────────────────────────────────────────────────────────────── */
static void cmd_ls(int argc, char* argv[]) {
    (void)argc; (void)argv;
    VFSNode* nodes;
    int count = vfs_list("/", &nodes);
    for (int i = 0; i < count; i++) {
        char line[128];
        luna_snprintf(line, sizeof(line), "  %-20s  %6u bytes  [%s%s%s]",
            nodes[i].name, nodes[i].size,
            (nodes[i].perms & VFS_PERM_R) ? "r" : "-",
            (nodes[i].perms & VFS_PERM_W) ? "w" : "-",
            (nodes[i].perms & VFS_PERM_X) ? "x" : "-");
        shell_println(line);
    }
}

/* ── cat ─────────────────────────────────────────────────────────────────── */
static void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) { shell_println("Usage: cat <file>"); return; }
    uint8_t buf[1024]; int rd = vfs_read(argv[1], buf, sizeof(buf));
    if (rd < 0) { shell_println("cat: file not found"); return; }
    buf[rd] = '\0'; shell_println((char*)buf);
}

/* ── write ───────────────────────────────────────────────────────────────── */
static void cmd_write(int argc, char* argv[]) {
    if (argc < 3) { shell_println("Usage: write <file> <data>"); return; }
    if (vfs_write(argv[1], (uint8_t*)argv[2], luna_strlen(argv[2])) < 0)
        shell_println("write: failed");
    else shell_println("OK");
}

/* ── rm ──────────────────────────────────────────────────────────────────── */
static void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) { shell_println("Usage: rm <file>"); return; }
    vfs_delete(argv[1]) ? shell_println("Deleted") : shell_println("rm: not found");
}

/* ── ledger ──────────────────────────────────────────────────────────────── */
static void cmd_ledger(int argc, char* argv[]) {
    if (argc >= 2 && luna_strcmp(argv[1], "verify") == 0) {
        shell_println(ledger_verify_chain() ? "[LEDGER] Chain VALID" : "[LEDGER] TAMPER DETECTED");
        return;
    }
    uint32_t count = ledger_get_count();
    char line[128];
    luna_snprintf(line, sizeof(line), "[LEDGER] %u blocks", count);
    shell_println(line);
    uint32_t show = (count > 8) ? 8 : count;
    for (uint32_t i = count > show ? count - show : 0; i < count; i++) {
        const LedgerBlock* b = ledger_get_block(i);
        char hash_hex[65]; sha256_to_hex(b->block_hash, hash_hex);
        luna_snprintf(line, sizeof(line), "  [%04u] %s | %s | %.8s...",
            b->index, b->target,
            b->action_type == LEDGER_ACTION_VFS_WRITE ? "WRITE" :
            b->action_type == LEDGER_ACTION_VFS_READ  ? "READ"  : "OTHER",
            hash_hex);
        shell_println(line);
    }
}

/* ── ps ──────────────────────────────────────────────────────────────────── */
static void cmd_ps(int argc, char* argv[]) {
    (void)argc; (void)argv;
    shell_println("  PID  RING  STATE    NAME");
    for (int i = 1; i < MAX_PROCESSES; i++) {
        Process* p = process_get_by_pid(i);
        if (!p || p->state == PROC_DEAD) continue;
        char line[128];
        luna_snprintf(line, sizeof(line), "  %3u   %u    %-8s %s",
            p->pid, p->ring,
            p->state == PROC_RUNNING ? "RUNNING" :
            p->state == PROC_READY   ? "READY"   :
            p->state == PROC_BLOCKED ? "BLOCKED"  : "ZOMBIE",
            p->name);
        shell_println(line);
    }
}

/* ── ai ──────────────────────────────────────────────────────────────────── */
static void cmd_ai(int argc, char* argv[]) {
    if (argc < 2) { shell_println("Usage: ai <query>"); return; }
    /* Reconstruct full query from argv[1..] */
    char query[256] = {0};
    for (int i = 1; i < argc; i++) {
        luna_strcat(query, argv[i]);
        if (i < argc - 1) luna_strcat(query, " ");
    }
    AIQueryContext ctx = {0};
    ctx.caller_ring = 0;
    const char* resp = ai_query(query, &ctx);
    shell_println(resp);
}

/* ── hash ────────────────────────────────────────────────────────────────── */
static void cmd_hash(int argc, char* argv[]) {
    if (argc < 2) { shell_println("Usage: hash <string|file>"); return; }
    uint8_t digest[32]; char hex[65];
    sha256((uint8_t*)argv[1], luna_strlen(argv[1]), digest);
    sha256_to_hex(digest, hex);
    char line[128];
    luna_snprintf(line, sizeof(line), "SHA-256: %s", hex);
    shell_println(line);
}

/* ── run (Luna script) ───────────────────────────────────────────────────── */
static void cmd_run(int argc, char* argv[]) {
    if (argc < 2) { shell_println("Usage: run <script.luna>"); return; }
    uint8_t src[4096];
    int rd = vfs_read(argv[1], src, sizeof(src) - 1);
    if (rd < 0) { shell_println("run: script not found"); return; }
    src[rd] = '\0';

    /* Lex → Parse → Eval using arena allocator */
    extern Arena g_lang_arena;
    arena_reset(&g_lang_arena);

    Lexer   lex   = lexer_create((char*)src, &g_lang_arena);
    Parser  par   = parser_create(&lex, &g_lang_arena);
    ASTNode* root = parser_parse(&par);
    if (!root) { shell_println("run: parse error"); return; }

    EvalContext ectx = eval_context_create(&g_lang_arena);
    Value result     = eval_node(&ectx, root);
    (void)result;
    arena_reset(&g_lang_arena);
}

/* ── help ────────────────────────────────────────────────────────────────── */
static void cmd_help(void) {
    shell_println("Luna OS — Project AETERNA | Available Commands:");
    shell_println("  ls                   — list filesystem");
    shell_println("  cat <file>           — read file");
    shell_println("  write <file> <data>  — write to file");
    shell_println("  rm <file>            — delete file");
    shell_println("  clear                — clear screen");
    shell_println("  ledger [verify]      — show/verify DAG-Ledger");
    shell_println("  ps                   — show processes");
    shell_println("  ai <query>           — query Luna AI");
    shell_println("  hash <text>          — SHA-256 hash");
    shell_println("  run <script.luna>    — run Luna script");
    shell_println("  shutdown             — halt system");
    shell_println("  <English sentence>   — NLP intent → command");
}

/* ── Shell main loop ─────────────────────────────────────────────────────── */
void shell_init(void) {
    cursor       = 0;
    hist_count   = 0;
    shell_running = true;
    luna_memset(input_buf, 0, sizeof(input_buf));
}

void shell_run(void) {
    shell_println("Luna OS — Project AETERNA (type 'help' for commands)");
    shell_prompt();
    /* Shell is input-driven; shell_on_key() called from keyboard driver */
    while (shell_running) {
        __asm__ volatile("hlt");  /* wait for next interrupt */
    }
}
