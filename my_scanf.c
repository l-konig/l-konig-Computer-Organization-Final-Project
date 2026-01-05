// Leora Konig
// COMP 2113 Final Project -- my_scanf

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

/* =========================
   BASIC HELPERS
   ========================= */
// Reads and discards leading whitespace characters from stdin.
// Stops at first non-whitespace character or EOF.
void skip_whitespace(void) {
    int ch;
    while ((ch = getchar()) != EOF && isspace(ch)) { }
    if (ch != EOF) ungetc(ch, stdin);                   // discard
}

// Returns next character in stdin without consuming it.
// Returns EOF if no input remains.
int peek_char(void) {
    int ch = getchar();
    if (ch != EOF) ungetc(ch, stdin);
    return ch;
}

// Attempt to match a single literal character from input.
// Returns 1 if next character matches expected literal, else 0.
// Non-matching characters are returned to stdin.
int match_literal(char expected) {
    int ch = getchar();
    if (ch != expected) {                  // If the character doesn't match the literal
        if (ch != EOF) ungetc(ch, stdin);
        return 0;
    }
    return 1;
}

// Compares two strings ignoring case. Returns 1 if identical, 0 if not.
int str_eq_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 0;  // Convert to lowercase to ignore case
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

/* =========================
   STORAGE HELPERS
   ========================= */
// Store a signed integer value into the destination pointer, honoring length modifier.
void store_signed_integer(void *ptr, const char *length, long long value) {
    if (length) {
        // Narrow or widen based on length modifier
        if (strcmp(length, "hh") == 0)
            *(signed char*)ptr = (signed char)value;  // %hhd
        else if (strcmp(length, "h") == 0)
            *(short*)ptr = (short)value;              // %hd
        else if (strcmp(length, "l") == 0)
            *(long*)ptr = (long)value;                // %ld
        else if (strcmp(length, "ll") == 0)
            *(long long*)ptr = value;                 // %lld
        else
            *(int*)ptr = (int)value;                  // Default integer
    } else {
        // No length modifier defaults to int
        *(int*)ptr = (int)value;
    }
}

// Apply a parsed sign and store the resulting integer value.
int store_integer_with_sign(void *ptr, const char *length, long long val, int sign) {
    val *= sign;   // Apply '+' or '-' determined during parsing
    store_signed_integer(ptr, length, val);
    return 1;      // Assignment succeeded
}

/* =========================
   DIGIT & SIGN HELPERS
   ========================= */
// Reads up to `width` digits from stdin in a given numeric base.
// Stores result in *value, returns 1 if at least 1 digit read.
int scan_digits_width(long long *value, int base, long long width) {
    int ch, count = 0;
    long long val = 0;

    // Consume characters while width allows and input is valid for the base
    while ((width == 0 || count < width) && (ch = getchar()) != EOF) {
        int digit = -1;

        // Convert character to numeric digit value
        if (isdigit(ch))
            digit = ch - '0';
        else if (isalpha(ch))
            digit = tolower(ch) - 'a' + 10;

        // Stop at first invalid digit and return it to the stream
        if (digit < 0 || digit >= base) {
            ungetc(ch, stdin);
            break;
        }

        // Detect overflow before multiplication/addition
        // Clamp to LLONG_MAX to mimic scanf-style saturation behavior
        if (val > (LLONG_MAX - digit) / base)
            val = LLONG_MAX;
        else
            val = val * base + digit;

        count++;
    }

    // No digits read → conversion failed
    if (count == 0) return 0;
    *value = val;
    return 1;
}

// Applies optional scientific notation ('e' or 'E') to an already-parsed float.
// Returns 0 only if an exponent marker is present but malformed.
static int apply_exponent(double *result) {
    int ch = peek_char();

    // No exponent → nothing to apply
    if (ch != 'e' && ch != 'E') return 1;

    getchar();    // Consume 'e' or 'E'

    // Optional exponent sign
    int exp_sign = 1;
    ch = peek_char();
    if (ch == '+') getchar();
    else if (ch == '-') { getchar(); exp_sign = -1; }

    int digits = 0;
    int exponent = 0;

    // Parse exponent digits
    while (isdigit(ch = getchar())) {
        exponent = exponent*10 + (ch-'0');
        digits++;
    }

    // Put back first non-digit character
    if (ch != EOF) ungetc(ch, stdin);

    // 'e' present but no digits → invalid exponent
    if (digits == 0) return 0;

    // Apply exponent as power of 10
    *result *= pow(10.0, exp_sign*exponent);
    return 1;
}

/* =========================
   SCAN FUNCTIONS
   ========================= */

// Parses a signed decimal integer (%d).
// Returns 1 on successful conversion.
// Handles optional sign, width limiting, overflow saturation,
// and stores according to the length modifier.
int scan_int(void *ptr, int width, const char *length) {
    skip_whitespace();  // scanf skips leading whitespace for numeric conversions

    int sign = 1;
    int ch = peek_char();

    // Optional sign handling
    if (ch == '+' || ch == '-') {
        getchar();                      // consume sign
        sign = (ch == '-') ? -1 : 1;
        if (width > 0) width--;         // sign counts toward width
    }

    long long value = 0;
    int digits_read = 0;

    // Read digits while respecting width
    while ((width == 0 || digits_read < width) &&
           (ch = getchar()) != EOF && isdigit(ch)) {

        int digit = ch - '0';

        // Detect and saturate on overflow
        if (value > (LLONG_MAX - digit) / 10)
            value = LLONG_MAX;
        else
            value = value * 10 + digit;

        digits_read++;
    }

    // No digits read → conversion failure
    if (digits_read == 0) {
        if (ch != EOF) ungetc(ch, stdin);
        return 0;
    }

    // Put back the first non-digit character
    if (ch != EOF && !isdigit(ch))
        ungetc(ch, stdin);

    // Apply sign and store into destination
    return store_integer_with_sign(ptr, length, value, sign);
}

// Parses a hexadecimal integer (%x / %X).
// Returns 1 on successful conversion.
// Accepts optional 0x / 0X prefix and respects width.
int scan_hex(void *ptr, int width, const char *length) {
    skip_whitespace();

    int digits = 0;
    long long val = 0;
    int ch = getchar();

    // Optional leading 0x / 0X prefix
    if (ch == '0') {
        digits = 1;                     // count leading zero
        val = 0;
        int next = getchar();
        if (next == 'x' || next == 'X') {
            // prefix fully consumed
        } else {
            ungetc(next, stdin);        // not actually a prefix
        }
    } else {
        ungetc(ch, stdin);
    }

    // Consume hexadecimal digits
    while ((ch = getchar()) != EOF) {
        int d;

        if ('0' <= ch && ch <= '9') d = ch - '0';
        else if ('a' <= ch && ch <= 'f') d = ch - 'a' + 10;
        else if ('A' <= ch && ch <= 'F') d = ch - 'A' + 10;
        else {
            ungetc(ch, stdin);          // non-hex character
            break;
        }

        val = val * 16 + d;
        digits++;

        if (width && digits >= width) break;
    }

    if (digits == 0) return 0;

    store_signed_integer(ptr, length, val);
    return 1;
}

// Parses a binary integer (%b extension).
// RETURN VALUE:
//   1 if at least one binary digit was parsed
//   0 if input was not binary
//  -1 if EOF encountered before input
// Accepts optional 0b / 0B prefix.
int scan_binary(int *value) {
    int ch;
    int result = 0;
    int found_digit = 0;

    // Skip leading whitespace
    while ((ch = getchar()) != EOF &&
           (ch == ' ' || ch == '\t' || ch == '\n')) { }

    if (ch == EOF) {
        *value = 0;
        return -1;
    }

    // Optional 0b / 0B prefix
    if (ch == '0') {
        int next = getchar();
        if (next == 'b' || next == 'B')
            ch = getchar();
        else {
            ungetc(next, stdin);
            ch = '0';
        }
    }

    // Consume binary digits
    while (ch == '0' || ch == '1') {
        found_digit = 1;
        result = (result << 1) | (ch - '0');
        ch = getchar();
    }

    if (ch != EOF) ungetc(ch, stdin);

    *value = result;
    return found_digit ? 1 : 0;
}

// Parses a floating-point value (%f).
// Returns 1 on successful conversion.
// Supports optional sign, fractional part, and scientific notation.
int scan_float(double *ptr, int width) {
    skip_whitespace();

    int ch = peek_char();
    if (ch == EOF) return 0;

    int sign = 1;

    // Optional sign
    if (ch == '+' || ch == '-') {
        if (getchar() == '-') sign = -1;
        if (width) width--;
    }

    double result = 0.0;
    int digits_read = 0;

    // Integer portion
    while (isdigit(ch = peek_char()) &&
           (width == 0 || width-- > 0)) {
        getchar();
        result = result * 10 + (ch - '0');
        digits_read++;
    }

    // Fractional portion
    if (peek_char() == '.' && (width == 0 || width > 0)) {
        getchar();                      // consume '.'
        double divisor = 10.0;

        while (isdigit(ch = peek_char()) &&
               (width == 0 || width-- > 0)) {
            getchar();
            result += (ch - '0') / divisor;
            divisor *= 10.0;
            digits_read++;
        }
    }

    if (digits_read == 0) return 0;

    // Optional exponent (e / E)
    if (!apply_exponent(&result)) return 0;

    *ptr = result * sign;
    return 1;
}

// Reads one or more raw characters (%c).
// Returns 1 on successful conversion.
// Does not skip whitespace unless width > 1.
int scan_char(char *c, int width) {
    int ch, count = 0;

    while (count < (width ? width : 1) &&
           (ch = getchar()) != EOF)
        c[count++] = (char)ch;

    return count ? 1 : 0;
}

// Reads a whitespace-delimited string (%s).
// Returns 1 on successful conversion.
// Skips leading whitespace and stops at first space.
int scan_string(char *buf, int max_width) {
    int ch, count = 0;

    skip_whitespace();

    while ((ch = getchar()) != EOF &&
           !isspace(ch) &&
           count < max_width)
        buf[count++] = (char)ch;

    if (ch != EOF && isspace(ch))
        ungetc(ch, stdin);

    buf[count] = '\0';
    return count > 0;
}

// Reads characters until a delimiter sequence is matched (%D).
// RETURN VALUE:
//   1 on successful read
//   0 if no characters were read
//  -1 on EOF with no input
// Supports multi-character delimiters
// using sliding-window matching.
int scan_delimited_string(char *buf, int max_width, const char *delimiter) {
    int ch, count = 0;
    size_t delim_len = strlen(delimiter);
    char window[128];
    size_t win_count = 0;

    if (delim_len >= sizeof(window)) return 0;

    while ((ch = getchar()) != EOF && count < max_width) {
        // Empty line → no conversion
        if (count == 0 && ch == '\n') {
            buf[0] = '\0';
            ungetc(ch, stdin);
            return 0;
        }

        buf[count++] = (char)ch;

        // Sliding window for delimiter detection
        if (delim_len > 0) {
            if (win_count < delim_len)
                window[(int)(win_count++)] = (char)ch;
            else {
                memmove(window, window + 1, delim_len - 1);
                window[(int)(delim_len - 1)] = (char)ch;
            }

            // Full delimiter matched → stop
            if (win_count == delim_len &&
                strncmp(window, delimiter, delim_len) == 0) {
                count -= (int)delim_len;
                break;
            }
        }

        // Single-character whitespace delimiter handling
        if (delim_len == 1 &&
            delimiter[0] != ch &&
            (ch == ' ' || ch == '\t' || ch == '\n')) {
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

    // Trim trailing newline
    if (count > 0 && buf[count - 1] == '\n') {
        buf[count - 1] = '\0';
        count--;
    }

    return count > 0 ? 1 : 0;
}

// Parses boolean-like textual values (%B).
// RETURN VALUE:
//   1 if a valid boolean token was parsed
//   0 otherwise
// Accepts common true/false spellings and numeric equivalents.
int scan_bool(int *value) {
    skip_whitespace();

    char buf[256];
    if (!scan_string(buf, 255)) {
        *value = 0;
        return 0;
    }

    // True values
    if (str_eq_ignore_case(buf, "true") ||
        str_eq_ignore_case(buf, "yes")  ||
        str_eq_ignore_case(buf, "on")   ||
        strcmp(buf, "1") == 0) {
        *value = 1;
        return 1;
    }

    // False values
    if (str_eq_ignore_case(buf, "false") ||
        str_eq_ignore_case(buf, "no")    ||
        str_eq_ignore_case(buf, "off")   ||
        strcmp(buf, "0") == 0) {
        *value = 0;
        return 1;
    }

    *value = 0;
    return 0;
}

/* =========================
   my_scanf
   ========================= */
// Custom scanf implementation supporting standard conversions and extensions (%b, %D, %B).
// Returns number of successfully assigned input items.
// Returns 0 if no assignments could be made, EOF if input ended before any assignments.
int my_scanf(const char *format, ...) {
    // Variable argument list
    va_list args;
    // Initialize it
    va_start(args, format);
    // Count of successfully assigned conversions
    int assigned = 0;

    // Loop through each character of the format string
    for (const char *p = format; *p; p++) {
        // Conversion specifier
        if (*p == '%') {
            p++;                // Move past '%'
            int suppress = 0;   // Suppression flag (if '*' is present)

            // Check for suppression operator '*'
            if (*p == '*') { suppress = 1; p++; }

            // Handle literal "%%" (matches a single '%' in input)
            if (*p == '%') {
                int ch = getchar();                 // Read next input character
                if (ch == EOF) {                    // End of input
                    va_end(args);
                    return assigned ? assigned : EOF;
                }
                if (ch != '%') {                    // Did not match '%'
                    ungetc(ch, stdin);             // Put character back
                    va_end(args);
                    return assigned ? assigned : 0;
                }
                continue;                           // Matched literal '%', continue
            }

            // Optional field width parsing (e.g., %10s)
            int width = 0;
            while (isdigit(*p)) {
                width = width * 10 + (*p - '0');  // Accumulate width
                p++;
            }

            // Optional length modifiers: h, hh, l, ll
            char length[3] = "";
            if (*p == 'h' && *(p+1) == 'h') { strcpy(length,"hh"); p+=2; }
            else if (*p == 'h') { strcpy(length,"h"); p++; }
            else if (*p == 'l' && *(p+1) == 'l') { strcpy(length,"ll"); p+=2; }
            else if (*p == 'l') { strcpy(length,"l"); p++; }

            char spec = *p;           // Conversion specifier character
            if (!spec) break;         // End of format string

            // Switch based on conversion specifier
            switch (spec) {
                case 'd': { // Signed decimal integer
                    if (suppress) {
                        int discard;
                        if (!scan_int(&discard, width, length)) goto end;
                    } else {
                        void *arg = va_arg(args, void*);
                        if (!scan_int(arg, width, length)) goto end;
                        assigned++;
                    }
                    break;
                }

                case 'x': {
                    // Hexadecimal integer
                    if (suppress) {
                        int discard;
                        if (!scan_hex(&discard, width, length)) goto end;
                    } else {
                        void *arg = va_arg(args, void*);
                        if (!scan_hex(arg, width, length)) goto end;
                        assigned++;
                    }
                    break;
                }
                case 'b': { // Binary integer (custom %b)
                    int tmp;
                    int *arg = suppress ? &tmp : va_arg(args,int*);
                    int ret = scan_binary(arg);       // Returns 1 on success, 0/negative on failure
                    if (ret <= 0) goto end;          // Stop on failure or EOF
                    if (!suppress) assigned++;
                    break;
                }
                case 'f': { // Floating-point number
                    if (strcmp(length,"l") == 0 || strcmp(length,"ll") == 0) {
                        double tmp;
                        double *arg = suppress ? &tmp : va_arg(args,double*);
                        if (!scan_float(arg,width)) goto end;
                        if (!suppress) assigned++;
                    } else {
                        double tmp;
                        float *arg = suppress ? NULL : va_arg(args,float*);
                        if (!scan_float(&tmp,width)) goto end;
                        if (!suppress) *arg = (float)tmp; // Assign float value
                        if (!suppress) assigned++;
                    }
                    break;
                }
                case 'c': { // Single character(s)
                    char tmp[8];
                    char *arg = suppress ? tmp : va_arg(args,char*);
                    if (!scan_char(arg,width)) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                case 's': { // String
                    char tmp[256];
                    char *arg = suppress ? tmp : va_arg(args,char*);
                    if (!scan_string(arg,width ? width : 256)) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                case 'D': { // Delimited string
                    char tmp[256];
                    char *arg = suppress ? tmp : va_arg(args,char*);
                    int w = width ? width : 256;
                    const char *delimiter = ",";           // Default delimiter
                    int ret = scan_delimited_string(arg, w, delimiter);
                    if (ret <= 0) goto end;               // Stop on failure or EOF
                    if (!suppress) assigned++;
                    break;
                }
                case 'B': { // Boolean
                    int tmp;
                    int *arg = suppress ? &tmp : va_arg(args,int*);
                    if (!scan_bool(arg)) goto end;
                    if (!suppress) assigned++;
                    break;
                }
                default: { // Literal character match
                    int ch = getchar();
                    if (ch == EOF) goto end;             // EOF stops reading
                    if (ch != spec) {                    // Mismatch → stop
                        ungetc(ch, stdin);
                        goto end;
                    }
                    break;
                }
            }
        } else if (isspace(*p)) {
            skip_whitespace(); // Any whitespace in format matches any whitespace in input
        } else {
            // Literal character in format
            int ch = getchar();
            if (ch == EOF) goto end;
            if (ch != *p) { ungetc(ch, stdin); goto end; }
        }
    }

end:
    va_end(args); // Clean up argument list
    return assigned ? assigned : (feof(stdin) ? EOF : 0); // Return assignments, 0, or EOF
}