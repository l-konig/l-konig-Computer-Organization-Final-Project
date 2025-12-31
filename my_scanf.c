#include <stdio.h>
#include <ctype.h>

/* HELPER FUNCTIONS */
/* Consume and discard all leading whitespace */
void skip_whitespace(void) {
    int ch;
    while ((ch = getchar()) != EOF && isspace(ch))
        ;
    if (ch != EOF) ungetc(ch, stdin);
}

/* Look at next character without consuming it */
int peek_char(void) {
    int ch = getchar();
    if (ch != EOF) ungetc(ch, stdin);
    return ch;
}

/* Match a literal character from input */
int match_literal(char expected) {
    int ch = getchar();
    if (ch != expected) {
        if (ch != EOF) ungetc(ch, stdin);
        return 0;
    }
    return 1;
}

/* Read optional '+' or '-' sign; returns 1 for '+', -1 for '-', 1 if none */
int read_sign(void) {
    int ch = getchar();
    if (ch == '-') return -1;
    if (ch == '+') return 1;
    if (ch != EOF) ungetc(ch, stdin);
    return 1;
}

/* Generic digit reader: base = 10, 16, 2 */
int read_digits(int *value, int base) {
    int ch;
    int count = 0;
    *value = 0;

    while ((ch = getchar()) != EOF) {
        int digit;
        if (isdigit(ch)) digit = ch - '0';
        else if (isalpha(ch)) digit = tolower(ch) - 'a' + 10;
        else break;

        if (digit >= base) break;
        *value = (*value * base) + digit;
        count++;
    }

    if (ch != EOF) ungetc(ch, stdin);
    return count;
}

/* Read digits after decimal point for floats */
double read_fractional_part(void) {
    int ch;
    double frac = 0.0, divisor = 10.0;

    while ((ch = getchar()) != EOF && isdigit(ch)) {
        frac += (ch - '0') / divisor;
        divisor *= 10.0;
    }

    if (ch != EOF) ungetc(ch, stdin);
    return frac;
}


/* Case-insensitive string comparison */
int str_eq_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}


/* Scan signed decimal integer */
int scan_int(int *num) {
    skip_whitespace();
    int sign = read_sign();
    if (!isdigit(peek_char())) return 0;

    int value;
    if (!read_digits(&value, 10)) return 0;

    *num = sign * value;
    return 1;
}

/* Scan hexadecimal integer */
int scan_hex(int *num) {
    skip_whitespace();
    if (!isxdigit(peek_char())) return 0;

    int value;
    if (!read_digits(&value, 16)) return 0;
    *num = value;
    return 1;
}

/* Scan binary integer */
int scan_binary(int *num) {
    skip_whitespace();
    int next = peek_char();
    if (next != '0' && next != '1') return 0;

    int value;
    if (!read_digits(&value, 2)) return 0;
    *num = value;
    return 1;
}

/* Scan floating-point number (supports optional sign, integer part, fractional part) */
int scan_float(double *num) {
    skip_whitespace();
    int sign = read_sign();
    int int_part = 0;
    double frac_part = 0.0;

    int next = peek_char();
    if (!isdigit(next) && next != '.') return 0;

    read_digits(&int_part, 10);

    if (peek_char() == '.') {
        getchar();  // consume '.'
        frac_part = read_fractional_part();
    }

    *num = sign * (int_part + frac_part);
    return 1;
}

/* Scan a single character (does NOT skip whitespace) */
int scan_char(char *c) {
    int ch = getchar();
    if (ch == EOF) return 0;
    *c = (char)ch;
    return 1;
}

/* Scan a word (non-whitespace string) */
int scan_string(char *buf) {
    skip_whitespace();
    int ch, count = 0;

    while ((ch = getchar()) != EOF && !isspace(ch)) {
        buf[count++] = ch;
    }
    buf[count] = '\0';
    if (count == 0 && ch == EOF) return 0;
    return 1;
}

/* Scan quoted string (supports spaces) */
int scan_quoted_string(char *buf) {
    skip_whitespace();
    int ch = getchar();
    if (ch != '"') { if (ch != EOF) ungetc(ch, stdin); return 0; }

    int count = 0;
    while ((ch = getchar()) != EOF && ch != '"') buf[count++] = ch;
    buf[count] = '\0';
    return (ch == '"');
}

/* Scan boolean (accepts true/false, yes/no, on/off, 1/0) */
int scan_bool(int *value) {
    char buf[16];
    if (!scan_string(buf)) return 0;

    if (str_eq_ignore_case(buf, "true") ||
        str_eq_ignore_case(buf, "yes") ||
        str_eq_ignore_case(buf, "on") ||
        (buf[0] == '1' && buf[1] == '\0')) {
        *value = 1; return 1;
    }

    if (str_eq_ignore_case(buf, "false") ||
        str_eq_ignore_case(buf, "no") ||
        str_eq_ignore_case(buf, "off") ||
        (buf[0] == '0' && buf[1] == '\0')) {
        *value = 0; return 1;
    }

    return 0;
}
