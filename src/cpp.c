/* A tiny interpreter for a subset of C++. It is intentionally small: enough
   to type a real-looking program (cout, int variables, arithmetic) into the
   /cpp window and watch it run. It is not a real compiler. */
#include "cpp.h"
#include "util.h"
#include "types.h"

#define MAX_VARS 32
#define NAME_LEN 16

typedef struct {
    char names[MAX_VARS][NAME_LEN];
    int  values[MAX_VARS];
    int  count;
    const char *p;
    const cpp_out_t *out;
} interp_t;

static int is_space(char c)  { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static int is_digit(char c)  { return c >= '0' && c <= '9'; }
static int is_alpha(char c)  { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int is_alnum(char c)  { return is_alpha(c) || is_digit(c); }

static void skip_spaces(interp_t *it) {
    while (*it->p && is_space(*it->p))
        it->p++;
}

static int var_index(interp_t *it, const char *name) {
    for (int i = 0; i < it->count; i++)
        if (str_eq(it->names[i], name))
            return i;
    return -1;
}

static void var_set(interp_t *it, const char *name, int value) {
    int idx = var_index(it, name);
    if (idx >= 0) {
        it->values[idx] = value;
        return;
    }
    if (it->count < MAX_VARS) {
        str_copy(it->names[it->count], name, NAME_LEN);
        it->values[it->count] = value;
        it->count++;
    }
}

static int var_get(interp_t *it, const char *name) {
    int idx = var_index(it, name);
    return idx >= 0 ? it->values[idx] : 0;
}

/* Read an identifier into buf (bounded); returns length read. */
static int read_ident(interp_t *it, char *buf, int cap) {
    int n = 0;
    while (is_alnum(*it->p) && n + 1 < cap)
        buf[n++] = *it->p++;
    buf[n] = '\0';
    return n;
}

/* ---- expression evaluator: + - * / % with parentheses and variables ---- */
static int parse_expr(interp_t *it);

static int parse_factor(interp_t *it) {
    skip_spaces(it);
    if (*it->p == '(') {
        it->p++;
        int v = parse_expr(it);
        skip_spaces(it);
        if (*it->p == ')')
            it->p++;
        return v;
    }
    if (*it->p == '-') {
        it->p++;
        return -parse_factor(it);
    }
    if (is_digit(*it->p)) {
        int v = 0;
        while (is_digit(*it->p))
            v = v * 10 + (*it->p++ - '0');
        return v;
    }
    if (is_alpha(*it->p)) {
        char name[NAME_LEN];
        read_ident(it, name, NAME_LEN);
        return var_get(it, name);
    }
    return 0;
}

static int parse_term(interp_t *it) {
    int v = parse_factor(it);
    for (;;) {
        skip_spaces(it);
        char op = *it->p;
        if (op != '*' && op != '/' && op != '%')
            return v;
        it->p++;
        int rhs = parse_factor(it);
        if (op == '*')      v = v * rhs;
        else if (op == '/') v = rhs ? v / rhs : 0;
        else                v = rhs ? v % rhs : 0;
    }
}

static int parse_expr(interp_t *it) {
    int v = parse_term(it);
    for (;;) {
        skip_spaces(it);
        char op = *it->p;
        if (op != '+' && op != '-')
            return v;
        it->p++;
        int rhs = parse_term(it);
        v = (op == '+') ? v + rhs : v - rhs;
    }
}

/* Skip the rest of the current statement (up to ';' or a block boundary). */
static void skip_statement(interp_t *it) {
    while (*it->p && *it->p != ';' && *it->p != '{' && *it->p != '}')
        it->p++;
    if (*it->p == ';')
        it->p++;
}

static void emit_int(interp_t *it, int value) {
    char buf[12];
    int_to_str(value, buf);
    it->out->text(buf);
}

/* cout << item << item ... ; */
static void do_cout(interp_t *it) {
    for (;;) {
        skip_spaces(it);
        if (*it->p != '<' || it->p[1] != '<')
            break;
        it->p += 2;
        skip_spaces(it);

        if (*it->p == '"') {
            char buf[128];
            int n = 0;
            it->p++;
            while (*it->p && *it->p != '"' && n + 1 < (int)sizeof(buf)) {
                char c = *it->p++;
                if (c == '\\' && *it->p) {
                    char e = *it->p++;
                    if (e == 'n') { buf[n] = '\0'; it->out->text(buf); it->out->newline(); n = 0; continue; }
                    else if (e == 't') c = '\t';
                    else c = e;
                }
                buf[n++] = c;
            }
            buf[n] = '\0';
            if (*it->p == '"')
                it->p++;
            it->out->text(buf);
        } else if (is_alpha(*it->p)) {
            /* peek identifier: endl is special, otherwise treat as expression */
            const char *save = it->p;
            char name[NAME_LEN];
            read_ident(it, name, NAME_LEN);
            if (str_eq(name, "endl")) {
                it->out->newline();
            } else {
                it->p = save;
                emit_int(it, parse_expr(it));
            }
        } else {
            emit_int(it, parse_expr(it));
        }
    }
    skip_statement(it);
}

/* Handle a statement beginning with the "int" keyword: either a function
   header (int main(...) {) which we ignore, or a variable declaration. */
static void do_int(interp_t *it) {
    skip_spaces(it);
    char name[NAME_LEN];
    if (!is_alpha(*it->p)) { skip_statement(it); return; }
    read_ident(it, name, NAME_LEN);
    skip_spaces(it);

    if (*it->p == '(') {                 /* function header: skip to body */
        while (*it->p && *it->p != '{')
            it->p++;
        return;
    }
    if (*it->p == '=') {
        it->p++;
        var_set(it, name, parse_expr(it));
        skip_statement(it);
        return;
    }
    var_set(it, name, 0);                /* declaration with no initializer */
    skip_statement(it);
}

void cpp_run(const char *src, const cpp_out_t *out) {
    interp_t it;
    it.count = 0;
    it.p = src;
    it.out = out;

    while (*it.p) {
        skip_spaces(&it);
        char c = *it.p;
        if (c == '\0')
            break;
        if (c == '{' || c == '}' || c == ';') { it.p++; continue; }
        if (c == '#') {                          /* preprocessor line */
            while (*it.p && *it.p != '\n')
                it.p++;
            continue;
        }

        if (is_alpha(c)) {
            const char *save = it.p;
            char word[NAME_LEN];
            read_ident(&it, word, NAME_LEN);

            if (str_eq(word, "cout")) {
                do_cout(&it);
            } else if (str_eq(word, "int")) {
                do_int(&it);
            } else if (str_eq(word, "using") || str_eq(word, "return") ||
                       str_eq(word, "std")) {
                skip_statement(&it);
            } else {
                /* assignment "name = expr;" ? */
                skip_spaces(&it);
                if (*it.p == '=' && it.p[1] != '=') {
                    it.p++;
                    var_set(&it, word, parse_expr(&it));
                    skip_statement(&it);
                } else {
                    it.p = save;
                    skip_statement(&it);
                }
            }
        } else {
            skip_statement(&it);
        }
    }
}
