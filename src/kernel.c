/* Notepad-OS 32-bit kernel.
   A small protected-mode kernel: a notepad text editor drawn in VGA text
   mode, driven by a from-scratch PS/2 keyboard driver. Typing "/cpp" on a
   line opens a C++ runner window backed by the built-in interpreter. */
#include "types.h"
#include "util.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "cpp.h"

/* ------------------------------------------------------------------ */
/* Text editor buffer                                                 */
/* ------------------------------------------------------------------ */
#define ED_ROWS 200
#define ED_COLS 78

typedef struct {
    char text[ED_ROWS][ED_COLS + 1];
    int  len[ED_ROWS];
    int  nrows;
    int  cur_row, cur_col;
    int  top;          /* first visible row (scroll offset) */
} editor_t;

static void ed_init(editor_t *e) {
    mem_set(e, 0, sizeof(*e));
    e->nrows = 1;
}

static void ed_insert_char(editor_t *e, char c) {
    int r = e->cur_row;
    if (e->len[r] >= ED_COLS)
        return;
    for (int i = e->len[r]; i > e->cur_col; i--)
        e->text[r][i] = e->text[r][i - 1];
    e->text[r][e->cur_col] = c;
    e->len[r]++;
    e->cur_col++;
    e->text[r][e->len[r]] = '\0';
}

static void ed_newline(editor_t *e) {
    if (e->nrows >= ED_ROWS)
        return;
    /* shift lines below down by one */
    for (int i = e->nrows; i > e->cur_row + 1; i--) {
        str_copy(e->text[i], e->text[i - 1], ED_COLS + 1);
        e->len[i] = e->len[i - 1];
    }
    /* split the current line at the cursor */
    int r = e->cur_row;
    int tail = e->len[r] - e->cur_col;
    for (int i = 0; i < tail; i++)
        e->text[r + 1][i] = e->text[r][e->cur_col + i];
    e->text[r + 1][tail] = '\0';
    e->len[r + 1] = tail;
    e->text[r][e->cur_col] = '\0';
    e->len[r] = e->cur_col;

    e->nrows++;
    e->cur_row++;
    e->cur_col = 0;
}

static void ed_backspace(editor_t *e) {
    int r = e->cur_row;
    if (e->cur_col > 0) {
        for (int i = e->cur_col - 1; i < e->len[r]; i++)
            e->text[r][i] = e->text[r][i + 1];
        e->len[r]--;
        e->cur_col--;
    } else if (r > 0) {
        /* join with previous line */
        int prev = r - 1;
        int base = e->len[prev];
        if (base + e->len[r] <= ED_COLS) {
            for (int i = 0; i <= e->len[r]; i++)
                e->text[prev][base + i] = e->text[r][i];
            e->len[prev] = base + e->len[r];
        }
        for (int i = r; i < e->nrows - 1; i++) {
            str_copy(e->text[i], e->text[i + 1], ED_COLS + 1);
            e->len[i] = e->len[i + 1];
        }
        e->nrows--;
        e->cur_row--;
        e->cur_col = base;
    }
}

/* Flatten the buffer into `out`, joining rows with '\n' and skipping
   row `skip` (pass -1 to keep all rows). */
static void ed_serialize(editor_t *e, char *out, int cap, int skip) {
    int n = 0;
    for (int r = 0; r < e->nrows; r++) {
        if (r == skip)
            continue;
        for (int i = 0; i < e->len[r] && n + 1 < cap; i++)
            out[n++] = e->text[r][i];
        if (n + 1 < cap)
            out[n++] = '\n';
    }
    out[n] = '\0';
}

/* ------------------------------------------------------------------ */
/* Interpreter output pane                                            */
/* ------------------------------------------------------------------ */
#define OUT_ROWS 64
static char out_text[OUT_ROWS][ED_COLS + 1];
static int  out_len[OUT_ROWS];
static int  out_nrows;

static void out_reset(void) {
    mem_set(out_text, 0, sizeof(out_text));
    mem_set(out_len, 0, sizeof(out_len));
    out_nrows = 1;
}

static void out_text_cb(const char *s) {
    int r = out_nrows - 1;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\t') {
            for (int t = 0; t < 2 && out_len[r] < ED_COLS; t++)
                out_text[r][out_len[r]++] = ' ';
            continue;
        }
        if (out_len[r] < ED_COLS)
            out_text[r][out_len[r]++] = s[i];
    }
    out_text[r][out_len[r]] = '\0';
}

static void out_newline_cb(void) {
    if (out_nrows < OUT_ROWS)
        out_nrows++;
}

static const cpp_out_t CPP_OUT = { out_text_cb, out_newline_cb };

/* ------------------------------------------------------------------ */
/* Rendering                                                          */
/* ------------------------------------------------------------------ */
static const u8 A_DESKTOP = 0;   /* filled in kmain (needs runtime calc) */

static void draw_titlebar(const char *title) {
    u8 bar = vga_attr(VGA_BLUE, VGA_LIGHT_GREY);
    vga_fill_rect(0, 0, VGA_WIDTH, 1, ' ', bar);
    vga_write(2, 0, title, bar);
}

static void draw_statusbar(const char *msg) {
    u8 bar = vga_attr(VGA_BLACK, VGA_LIGHT_GREY);
    vga_fill_rect(0, VGA_HEIGHT - 1, VGA_WIDTH, 1, ' ', bar);
    vga_write(1, VGA_HEIGHT - 1, msg, bar);
}

/* Draw an editor's visible rows inside a content area, updating scroll and
   placing the hardware cursor. */
static void draw_editor(editor_t *e, int x, int y, int w, int h, u8 attr) {
    if (e->cur_row < e->top)
        e->top = e->cur_row;
    if (e->cur_row >= e->top + h)
        e->top = e->cur_row - h + 1;

    for (int row = 0; row < h; row++) {
        int r = e->top + row;
        for (int col = 0; col < w; col++) {
            char c = (r < e->nrows && col < e->len[r]) ? e->text[r][col] : ' ';
            vga_put(x + col, y + row, c, attr);
        }
    }
    vga_set_cursor(x + e->cur_col, y + (e->cur_row - e->top));
}

/* ------------------------------------------------------------------ */
/* Command helpers                                                    */
/* ------------------------------------------------------------------ */
/* Return a pointer to the current line trimmed of leading/trailing spaces,
   copied into `buf`. */
static void current_line_trimmed(editor_t *e, char *buf, int cap) {
    int r = e->cur_row;
    int start = 0, end = e->len[r];
    while (start < end && e->text[r][start] == ' ')
        start++;
    while (end > start && e->text[r][end - 1] == ' ')
        end--;
    int n = 0;
    for (int i = start; i < end && n + 1 < cap; i++)
        buf[n++] = e->text[r][i];
    buf[n] = '\0';
}

/* ------------------------------------------------------------------ */
/* Notepad and C++ screens                                            */
/* ------------------------------------------------------------------ */
static editor_t notepad;
static editor_t code;

static void render_notepad(void) {
    u8 desktop = vga_attr(VGA_WHITE, VGA_BLUE);
    u8 border  = vga_attr(VGA_LIGHT_CYAN, VGA_BLUE);
    vga_fill(desktop);
    draw_titlebar("Notepad-OS  -  32-bit protected mode");
    vga_box(0, 1, VGA_WIDTH, VGA_HEIGHT - 2, border);
    vga_write(2, 1, " notepad ", vga_attr(VGA_YELLOW, VGA_BLUE));
    draw_editor(&notepad, 1, 2, ED_COLS, VGA_HEIGHT - 4, desktop);
    draw_statusbar("Type freely  |  /cpp = C++ runner  /clear = wipe  /help = help");
}

static void render_cpp(void) {
    u8 desktop = vga_attr(VGA_WHITE, VGA_BLUE);
    u8 border  = vga_attr(VGA_LIGHT_CYAN, VGA_BLUE);
    u8 outattr = vga_attr(VGA_LIGHT_GREEN, VGA_BLUE);
    vga_fill(desktop);
    draw_titlebar("Notepad-OS  -  C++ Runner");

    /* code window (top) */
    vga_box(0, 1, VGA_WIDTH, 14, border);
    vga_write(2, 1, " code.cpp ", vga_attr(VGA_YELLOW, VGA_BLUE));
    draw_editor(&code, 1, 2, ED_COLS, 12, desktop);

    /* output window (bottom) */
    vga_box(0, 15, VGA_WIDTH, VGA_HEIGHT - 16, border);
    vga_write(2, 15, " output ", vga_attr(VGA_YELLOW, VGA_BLUE));
    int oh = VGA_HEIGHT - 18;
    int start = out_nrows > oh ? out_nrows - oh : 0;
    for (int row = 0; row < oh; row++) {
        int r = start + row;
        for (int col = 0; col < ED_COLS; col++) {
            char c = (r < out_nrows && col < out_len[r]) ? out_text[r][col] : ' ';
            vga_put(1 + col, 16 + row, c, outattr);
        }
    }
    draw_statusbar("Write C++  |  /run = execute  /exit = back to notepad");
}

static void run_code(void) {
    char src[ED_ROWS * 8];
    out_reset();
    ed_serialize(&code, src, sizeof(src), code.cur_row); /* skip the /run line */
    cpp_run(src, &CPP_OUT);
}

static void seed_code_sample(void) {
    ed_init(&code);
    const char *sample[] = {
        "#include <iostream>",
        "using namespace std;",
        "int main() {",
        "    int a = 6;",
        "    int b = 7;",
        "    cout << \"a * b = \" << a * b << endl;",
        "    cout << \"hello from Notepad-OS\" << endl;",
        "    return 0;",
        "}",
    };
    int lines = (int)(sizeof(sample) / sizeof(sample[0]));
    for (int i = 0; i < lines; i++) {
        str_copy(code.text[i], sample[i], ED_COLS + 1);
        code.len[i] = (int)str_len(sample[i]);
    }
    code.nrows = lines;
    code.cur_row = lines - 1;
    code.cur_col = code.len[code.cur_row];
}

/* ------------------------------------------------------------------ */
/* Kernel main                                                        */
/* ------------------------------------------------------------------ */
enum mode { MODE_NOTEPAD, MODE_CPP };

void kmain(void) {
    (void)A_DESKTOP;
    ed_init(&notepad);
    out_reset();
    seed_code_sample();

    enum mode mode = MODE_NOTEPAD;
    render_notepad();

    for (;;) {
        char c = keyboard_getchar();
        editor_t *e = (mode == MODE_NOTEPAD) ? &notepad : &code;

        if (c == KEY_ENTER) {
            char line[ED_COLS + 1];
            current_line_trimmed(e, line, sizeof(line));

            if (mode == MODE_NOTEPAD) {
                if (str_eq(line, "/cpp")) {
                    mode = MODE_CPP;
                    render_cpp();
                    continue;
                } else if (str_eq(line, "/clear")) {
                    ed_init(&notepad);
                    render_notepad();
                    continue;
                } else if (str_eq(line, "/help")) {
                    ed_init(&notepad);
                    const char *help[] = {
                        "Notepad-OS help:",
                        "  /cpp    open the C++ runner window",
                        "  /clear  erase the notepad",
                        "  just type to take notes!",
                    };
                    for (int i = 0; i < 4; i++) {
                        str_copy(notepad.text[i], help[i], ED_COLS + 1);
                        notepad.len[i] = (int)str_len(help[i]);
                    }
                    notepad.nrows = 4;
                    notepad.cur_row = 3;
                    notepad.cur_col = notepad.len[3];
                    render_notepad();
                    continue;
                }
                ed_newline(e);
                render_notepad();
            } else { /* MODE_CPP */
                if (str_eq(line, "/run")) {
                    run_code();
                    render_cpp();
                    continue;
                } else if (str_eq(line, "/exit")) {
                    mode = MODE_NOTEPAD;
                    render_notepad();
                    continue;
                }
                ed_newline(e);
                render_cpp();
            }
        } else if (c == KEY_BACKSPACE) {
            ed_backspace(e);
            (mode == MODE_NOTEPAD) ? render_notepad() : render_cpp();
        } else if (c == KEY_TAB) {
            ed_insert_char(e, ' ');
            ed_insert_char(e, ' ');
            (mode == MODE_NOTEPAD) ? render_notepad() : render_cpp();
        } else if (c == KEY_ESC) {
            if (mode == MODE_CPP) {
                mode = MODE_NOTEPAD;
                render_notepad();
            }
        } else if (c >= ' ' && c < 127) {
            ed_insert_char(e, c);
            (mode == MODE_NOTEPAD) ? render_notepad() : render_cpp();
        }
    }
}
