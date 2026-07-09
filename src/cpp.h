#ifndef NOTEPADOS_CPP_H
#define NOTEPADOS_CPP_H

/* Output sink for the interpreter: text() appends characters to the current
   line, newline() moves to the next line. */
typedef struct {
    void (*text)(const char *s);
    void (*newline)(void);
} cpp_out_t;

/* Interpret a small subset of C++ and stream the result to `out`.

   Supported:
     - #include / using / return / function headers are ignored as boilerplate
     - int NAME = EXPR;         (declaration, optional initializer)
     - NAME = EXPR;             (assignment)
     - cout << ITEM << ... ;    (ITEM: "string", endl, integer or expression)
     - integer expressions with + - * / % and parentheses over int variables

   Anything it does not understand is skipped rather than aborting. */
void cpp_run(const char *src, const cpp_out_t *out);

#endif
