#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

/* =========================
   BASIC HELPERS
   ========================= */
// Reads and discards whitespace characters from stdin.
// Stops at first non-whitespace character or EOF.
void skip_whitespace(void) {
    int ch;
    while ((ch = getchar()) != EOF && isspace(ch)) { }
    if (ch != EOF) ungetc(ch, stdin);
}

// Returns next character in stdin without consuming it.
int peek_char(void) {
    int ch = getchar();
    if (ch != EOF) ungetc(ch, stdin);
    return ch;
}

// Returns 1 if next character matches expected literal, else 0.
// Non-matching characters are returned to stdin.
int match_literal(char expected) {
    int ch = getchar();
    if (ch != expected) {
        if (ch != EOF) ungetc(ch, stdin);
        return 0;
    }
    return 1;
}

// Compares two strings ignoring case. Returns 1 if equal, 0 if not.
int str_eq_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

/* =========================
   STORAGE HELPERS
   ========================= */
// Writes a signed integer into ptr according to length modifier.
void store_signed_integer(void *ptr, const char *length, long long value) {
    if (!length || strcmp(length, "") == 0)
        *(int*)ptr = (int)value;
    else if (strcmp(length, "hh") == 0)
        *(signed char*)ptr = (signed char)value;
    else if (strcmp(length, "h") == 0)
        *(short*)ptr = (short)value;
    else if (strcmp(length, "l") == 0)
        *(long*)ptr = (long)value;
    else if (strcmp(length, "ll") == 0)
        *(long long*)ptr = value;
    else
        *(int*)ptr = (int)value;
}

// Applies sign and stores integer without clamping.
int store_integer_with_sign(void *ptr, const char *length, long long val, int sign) {
    val *= sign;
    store_signed_integer(ptr, length, val);
    return 1;
}

/* =========================
   DIGIT & SIGN HELPERS
   ========================= */
// Reads up to `width` digits from stdin in a given base.
// Stores result in *value, returns 1 if at least 1 digit read.
int scan_digits_width(long long *value, int base, long long width) {
    int ch, count = 0;
    long long val = 0;

    while ((width == 0 || count < width) && (ch = getchar()) != EOF) {
        int digit = -1;
        if (isdigit(ch)) digit = ch - '0';
        else if (isalpha(ch)) digit = tolower(ch) - 'a' + 10;

        if (digit < 0 || digit >= base) {
            ungetc(ch, stdin);
            break;
        }

        // Detect overflow but don't clamp (undefined in standard scanf)
        if (val > (LLONG_MAX - digit) / base)
            val = LLONG_MAX;
        else
            val = val * base + digit;

        count++;
    }

    if (count == 0) return 0;
    *value = val;
    return 1;
}

// Applies 'e' or 'E' scientific notation exponent to a floating-point number.
static void apply_exponent(double *result) {
    int ch = peek_char();
    if (ch != 'e' && ch != 'E') return;

    getchar(); // consume 'e'/'E'
    int exp_sign = 1;

    ch = peek_char();
    if (ch == '+') getchar();
    else if (ch == '-') { getchar(); exp_sign = -1; }

    if (!isdigit(peek_char())) return;

    int exponent = 0;
    while (isdigit(ch = getchar()))
        exponent = exponent * 10 + (ch - '0');

    ungetc(ch, stdin);
    *result *= pow(10.0, exp_sign * exponent);
}

/* =========================
   SCAN FUNCTIONS
   ========================= */
int scan_int(void *ptr, int width, const char *length) {
    skip_whitespace();

    int sign = 1;
    int ch = peek_char();
    if ((ch == '+' || ch == '-') && (width == 0 || width > 0)) {
        getchar();
        sign = (ch == '-') ? -1 : 1;
        if (width) width--;
    }

    long long value = 0;
    int digits_read = 0;
    while ((width == 0 || digits_read < width) && (ch = getchar()) != EOF && isdigit(ch)) {
        int digit = ch - '0';
        if (value > (LLONG_MAX - digit) / 10) value = LLONG_MAX; // overflow (undefined)
        else value = value * 10 + digit;
        digits_read++;
    }
    if (ch != EOF && !isdigit(ch)) ungetc(ch, stdin);
    if (digits_read == 0) return 0;

    return store_integer_with_sign(ptr, length, value, sign);
}

// Hexadecimal numbers: no negative sign allowed
int scan_hex(void *ptr, int width, const char *length) {
    skip_whitespace();

    int ch = getchar();
    // Optional 0x prefix
    if (ch == '0') {
        int next = getchar();
        if (next != 'x' && next != 'X') { ungetc(next, stdin); ungetc(ch, stdin); }
    } else { ungetc(ch, stdin); }

    long long val = 0;
    if (!scan_digits_width(&val, 16, width ? width : 0)) return 0;

    store_signed_integer(ptr, length, val);
    return 1;
}

// Binary numbers
int scan_binary(void *ptr, int width, const char *length) {
    skip_whitespace();

    int sign = 1;
    int ch = peek_char();
    if (ch == '+' || ch == '-') {
        getchar();
        if (ch == '-') sign = -1;
        if (width) width--;
    }

    ch = getchar();
    if (ch == '0') {
        int nxt = getchar();
        if (nxt == 'b' || nxt == 'B') {
            if (width) width -= 2;
            ch = getchar();
        } else { ungetc(nxt, stdin); }
    }

    long long val = 0;
    int digits = 0;
    while ((ch == '0' || ch == '1') && (width == 0 || digits < width)) {
        val = (val << 1) | (ch - '0');
        digits++;
        ch = getchar();
    }
    if (ch != EOF && !(ch == '0' || ch == '1')) ungetc(ch, stdin);
    if (digits == 0) return 0;

    return store_integer_with_sign(ptr, length, val, sign);
}

// Floating point numbers
int scan_float(double *ptr, int width) {
    skip_whitespace();
    int sign = 1;
    int ch = peek_char();
    if (ch == '+' || ch == '-') {
        if (ch == '-') sign = -1;
        getchar();
        if (width) width--;
    }
    if (!isdigit(peek_char()) && peek_char() != '.') return 0;

    double result = 0.0;
    while (isdigit(ch = peek_char()) && (width == 0 || width-- > 0)) { getchar(); result = result * 10 + (ch - '0'); }
    if (peek_char() == '.' && (width == 0 || width > 0)) {
        getchar();
        double divisor = 10.0;
        while (isdigit(ch = peek_char()) && (width == 0 || width-- > 0)) { getchar(); result += (ch - '0') / divisor; divisor *= 10.0; }
    }
    apply_exponent(&result);
    *ptr = result * sign;
    return 1;
}

// Single char
int scan_char(char *c, int width) {
    int ch, count = 0;
    while (count < (width ? width : 1) && (ch = getchar()) != EOF) c[count++] = (char)ch;
    return count ? 1 : 0;
}

// Whitespace-delimited string
int scan_string(char *buf, int max_width) {
    int ch, count = 0;
    skip_whitespace();
    while ((ch = getchar()) != EOF && !isspace(ch) && count < max_width) buf[count++] = (char)ch;
    if (ch != EOF && isspace(ch)) ungetc(ch, stdin);
    buf[count] = '\0';
    return count > 0;
}

// Delimited string
int scan_delimited_string(char *buf, int max_width, char delim) {
    skip_whitespace();
    int ch, count = 0;
    while ((ch = getchar()) != EOF && ch != delim && count < max_width) buf[count++] = (char)ch;
    buf[count] = '\0';
    return count > 0;
}

// Boolean: true/false, yes/no, on/off, 1/0
int scan_bool(int *value) {
    char buf[256];
    if (!scan_string(buf, 255)) return 0;

    if (str_eq_ignore_case(buf, "true") || str_eq_ignore_case(buf, "yes") ||
        str_eq_ignore_case(buf, "on") || strcmp(buf, "1") == 0) { *value = 1; return 1; }

    if (str_eq_ignore_case(buf, "false") || str_eq_ignore_case(buf, "no") ||
        str_eq_ignore_case(buf, "off") || strcmp(buf, "0") == 0) { *value = 0; return 1; }

    return 0;
}

/* =========================
   my_scanf
   ========================= */
int my_scanf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int assigned = 0;

    for (const char *p = format; *p; p++) {
        if (*p == '%') {
            p++;

            if (*p == '%') {          // "%%" literal case
                if (!match_literal('%')) goto end;
                assigned++;
                continue;
            }

            // Parse optional width
            int width = 0;
            while (isdigit(*p)) { width = width * 10 + (*p - '0'); p++; }

            // Parse length modifier
            char length[3] = "";
            if (*p == 'h' && *(p + 1) == 'h') { strcpy(length, "hh"); p += 2; }
            else if (*p == 'h') { strcpy(length, "h"); p++; }
            else if (*p == 'l' && *(p + 1) == 'l') { strcpy(length, "ll"); p += 2; }
            else if (*p == 'l') { strcpy(length, "l"); p++; }

            char spec = *p;
            if (!spec) break;

            switch (spec) {
                case 'd': case 'i': {
                    void *arg = va_arg(args, void*);
                    if (!scan_int(arg, width, length)) goto end;
                    assigned++; break;
                }
                case 'x': {
                    void *arg = va_arg(args, void*);
                    if (!scan_hex(arg, width, length)) goto end;
                    assigned++; break;
                }
                case 'b': {
                    void *arg = va_arg(args, void*);
                    if (!scan_binary(arg, width, length)) goto end;
                    assigned++; break;
                }
                case 'f': {
                    if (strcmp(length, "l") == 0 || strcmp(length, "ll") == 0) {
                        double *arg = va_arg(args, double*);
                        if (!scan_float(arg, width)) goto end;
                    } else {
                        float *arg = va_arg(args, float*);
                        double tmp;
                        if (!scan_float(&tmp, width)) goto end;
                        *arg = (float)tmp;
                    }
                    assigned++; break;
                }
                case 'c': {
                    char *arg = va_arg(args, char*);
                    if (!scan_char(arg, width)) goto end;
                    assigned++; break;
                }
                case 's': {
                    char *arg = va_arg(args, char*);
                    if (!scan_string(arg, width ? width : 256)) goto end;
                    assigned++; break;
                }
                case 'D': {
                    char *arg = va_arg(args, char*);
                    char delim = *(p + 1);
                    if (!delim) goto end;
                    p++;
                    if (!scan_delimited_string(arg, width ? width : 256, delim)) goto end;
                    assigned++; break;
                }
                case 'B': {
                    int *arg = va_arg(args, int*);
                    if (!scan_bool(arg)) goto end;
                    assigned++; break;
                }
                default:
                    if (!match_literal(spec)) goto end;
            }
        } else if (isspace(*p)) skip_whitespace();
        else if (!match_literal(*p)) goto end;
    }

end:
    va_end(args);
    return assigned ? assigned : (feof(stdin) ? EOF : 0);
}
