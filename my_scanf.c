#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

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
    if (length) {
        if (strcmp(length, "hh") == 0)
            *(signed char*)ptr = (signed char)value;
        else if (strcmp(length, "h") == 0)
            *(short*)ptr = (short)value;
        else if (strcmp(length, "l") == 0)
            *(long*)ptr = (long)value;
        else if (strcmp(length, "ll") == 0)
            *(long long*)ptr = value;
        else
            *(int*)ptr = (int)value;
    } else {
        *(int*)ptr = (int)value;
    }
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
static int apply_exponent(double *result) {
    int ch = peek_char();
    if (ch != 'e' && ch != 'E') return 1;

    getchar();

    int exp_sign = 1;
    ch = peek_char();
    if (ch == '+') getchar();
    else if (ch == '-') { getchar(); exp_sign = -1; }

    int digits = 0;
    int exponent = 0;
    while (isdigit(ch = getchar())) {
        exponent = exponent*10 + (ch-'0');
        digits++;
    }

    if (ch != EOF) ungetc(ch, stdin);

    if (digits == 0) return 0;

    *result *= pow(10.0, exp_sign*exponent);
    return 1;
}

/* =========================
   SCAN FUNCTIONS
   ========================= */
int scan_int(void *ptr, int width, const char *length) {
    skip_whitespace();

    int sign = 1;
    int ch = peek_char();

    if (ch == '+' || ch == '-') {
        getchar();
        sign = (ch == '-') ? -1 : 1;
        if (width > 0) width--;
    }

    long long value = 0;
    int digits_read = 0;

    while ((width == 0 || digits_read < width) && (ch = getchar()) != EOF && isdigit(ch)) {
        int digit = ch - '0';
        if (value > (LLONG_MAX - digit) / 10) value = LLONG_MAX;
        else value = value * 10 + digit;
        digits_read++;
    }

    if (digits_read == 0) {
        if (ch != EOF) ungetc(ch, stdin);
        return 0;
    }

    if (ch != EOF && !isdigit(ch)) ungetc(ch, stdin);

    return store_integer_with_sign(ptr, length, value, sign);
}

int scan_hex(void *ptr, int width, const char *length) {
    skip_whitespace();

    int digits = 0;
    long long val = 0;
    int ch = getchar();

    if (ch == '0') {
        digits = 1;
        val = 0;
        int next = getchar();
        if (next == 'x' || next == 'X') { }
        else ungetc(next, stdin);
    } else ungetc(ch, stdin);

    while ((ch = getchar()) != EOF) {
        int d;
        if ('0' <= ch && ch <= '9') d = ch - '0';
        else if ('a' <= ch && ch <= 'f') d = ch - 'a' + 10;
        else if ('A' <= ch && ch <= 'F') d = ch - 'A' + 10;
        else { ungetc(ch, stdin); break; }

        val = val * 16 + d;
        digits++;
        if (width && digits >= width) break;
    }

    if (digits == 0) return 0;

    store_signed_integer(ptr, length, val);
    return 1;
}

int scan_binary(int *value) {
    int ch;
    int result = 0;
    int found_digit = 0;

    while ((ch = getchar()) != EOF && (ch == ' ' || ch == '\t' || ch == '\n')) { }

    if (ch == EOF) { *value = 0; return -1; }

    if (ch == '0') {
        int next = getchar();
        if (next == 'b' || next == 'B') ch = getchar();
        else { ungetc(next, stdin); ch = '0'; }
    }

    while (ch == '0' || ch == '1') {
        found_digit = 1;
        result = (result << 1) | (ch - '0');
        ch = getchar();
    }

    if (ch != EOF) ungetc(ch, stdin);

    *value = result;
    return found_digit ? 1 : 0;
}

int scan_float(double *ptr, int width) {
    skip_whitespace();

    int ch = peek_char();
    if (ch == EOF) return 0;

    int sign = 1;
    if (ch == '+' || ch == '-') {
        if (getchar() == '-') sign = -1;
        if (width) width--;
    }

    double result = 0.0;
    int digits_read = 0;

    while (isdigit(ch = peek_char()) && (width == 0 || width-- > 0)) {
        getchar();
        result = result*10 + (ch-'0');
        digits_read++;
    }

    if (peek_char() == '.' && (width == 0 || width > 0)) {
        getchar();
        double divisor = 10.0;
        while (isdigit(ch = peek_char()) && (width == 0 || width-- > 0)) {
            getchar();
            result += (ch-'0')/divisor;
            divisor *= 10.0;
            digits_read++;
        }
    }

    if (digits_read == 0) return 0;
    if (!apply_exponent(&result)) return 0;

    *ptr = result * sign;
    return 1;
}

int scan_char(char *c, int width) {
    int ch, count = 0;
    while (count < (width ? width : 1) && (ch = getchar()) != EOF) c[count++] = (char)ch;
    return count ? 1 : 0;
}

int scan_string(char *buf, int max_width) {
    int ch, count = 0;
    skip_whitespace();
    while ((ch = getchar()) != EOF && !isspace(ch) && count < max_width) buf[count++] = (char)ch;
    if (ch != EOF && isspace(ch)) ungetc(ch, stdin);
    buf[count] = '\0';
    return count > 0;
}

int scan_delimited_string(char *buf, int max_width, const char *delimiter) {
    int ch, count = 0;
    size_t delim_len = strlen(delimiter);
    char window[128];
    size_t win_count = 0;  // use size_t to match delim_len

    if (delim_len >= sizeof(window)) return 0;

    while ((ch = getchar()) != EOF && count < max_width) {
        if (count == 0 && ch == '\n') {
            buf[0] = '\0';
            ungetc(ch, stdin);
            return 0;
        }

        buf[count++] = (char)ch;

        if (delim_len > 0) {
            if (win_count < delim_len) window[(int)(win_count++)] = (char)ch; // cast to int for indexing
            else {
                memmove(window, window + 1, delim_len - 1);
                window[(int)(delim_len - 1)] = (char)ch;
            }

            if (win_count == delim_len && strncmp(window, delimiter, delim_len) == 0) {
                count -= (int)delim_len;  // already safe
                break;
            }
        }

        // Handle single-character whitespace delimiters
        if (delim_len == 1 && (delimiter[0] != ch) && (ch == ' ' || ch == '\t' || ch == '\n')) {
            count--;
            ungetc(ch, stdin);
            break;
        }
    }

    if (ch == EOF && count == 0) {
        buf[0] = '\0';
        return -1;
    }

    buf[count] = '\0';
    if (count > 0 && buf[count - 1] == '\n') {
        buf[count - 1] = '\0';
        count--;
    }

    return count > 0 ? 1 : 0;
}

int scan_bool(int *value) {
    skip_whitespace();
    char buf[256];
    if (!scan_string(buf, 255)) { *value = 0; return 0; }

    if (str_eq_ignore_case(buf,"true") || str_eq_ignore_case(buf,"yes") ||
        str_eq_ignore_case(buf,"on") || strcmp(buf,"1")==0) { *value = 1; return 1; }

    if (str_eq_ignore_case(buf,"false") || str_eq_ignore_case(buf,"no") ||
        str_eq_ignore_case(buf,"off") || strcmp(buf,"0")==0) { *value = 0; return 1; }

    *value = 0;
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
            int suppress = 0;
            if (*p == '*') { suppress = 1; p++; }

            if (*p == '%') {
                int ch = getchar();
                if (ch == EOF) { va_end(args); return assigned ? assigned : EOF; }
                if (ch != '%') { ungetc(ch, stdin); va_end(args); return assigned ? assigned : 0; }
                continue;
            }

            int width = 0;
            while (isdigit(*p)) { width = width*10 + (*p - '0'); p++; }

            char length[3] = "";
            if (*p == 'h' && *(p+1)=='h') { strcpy(length,"hh"); p+=2; }
            else if (*p=='h') { strcpy(length,"h"); p++; }
            else if (*p=='l' && *(p+1)=='l') { strcpy(length,"ll"); p+=2; }
            else if (*p=='l') { strcpy(length,"l"); p++; }

            char spec = *p;
            if (!spec) break;

            switch(spec) {
                case 'd': case 'i': {
                    if (suppress) {
                        int dummy;
                        if (!scan_int(&dummy, width, length)) goto end;
                    } else {
                        void *arg = va_arg(args, void*);
                        if (!scan_int(arg, width, length)) goto end;
                        assigned++;
                    }
                    break;
                }
                case 'x': {
                    if (suppress) {
                        int dummy;
                        if (!scan_hex(&dummy, width, length)) goto end;
                    } else {
                        void *arg = va_arg(args, void*);
                        if (!scan_hex(arg, width, length)) goto end;
                        assigned++;
                    }
                    break;
                }
                case 'b': {
                    int tmp;
                    int *arg = suppress ? &tmp : va_arg(args,int*);
                    int ret = scan_binary(arg);
                    if (ret <= 0) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                case 'f': {
                    if (strcmp(length,"l")==0 || strcmp(length,"ll")==0) {
                        double tmp;
                        double *arg = suppress ? &tmp : va_arg(args,double*);
                        if (!scan_float(arg,width)) goto end;
                        if (!suppress) assigned++;
                    } else {
                        double tmp;
                        float *arg = suppress ? NULL : va_arg(args,float*);
                        if (!scan_float(&tmp,width)) goto end;
                        if (!suppress) *arg = (float)tmp;
                        if (!suppress) assigned++;
                    }
                    break;
                }
                case 'c': {
                    char tmp[8];
                    char *arg = suppress ? tmp : va_arg(args,char*);
                    if (!scan_char(arg,width)) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                case 's': {
                    char tmp[256];
                    char *arg = suppress ? tmp : va_arg(args,char*);
                    if (!scan_string(arg,width?width:256)) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                case 'D': {
                    char tmp[256];
                    char *arg = suppress ? tmp : va_arg(args,char*);
                    int w = width?width:256;
                    const char *delimiter = ",";
                    int ret = scan_delimited_string(arg,w,delimiter);
                    if (ret <= 0) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                case 'B': {
                    int tmp;
                    int *arg = suppress ? &tmp : va_arg(args,int*);
                    if (!scan_bool(arg)) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                default: {
                    int ch = getchar();
                    if (ch == EOF) goto end;
                    if (ch != spec) { ungetc(ch, stdin); goto end; }
                    break;
                }
            }
        } else if (isspace(*p)) skip_whitespace();
        else {
            int ch = getchar();
            if (ch == EOF) goto end;
            if (ch != *p) { ungetc(ch, stdin); goto end; }
        }
    }

end:
    va_end(args);
    return assigned ? assigned : (feof(stdin)?EOF:0);
}
