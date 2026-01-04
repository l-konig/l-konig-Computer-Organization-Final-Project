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

    int digits = 0;
    long long val = 0;

    int ch = getchar();

    // Handle leading 0
    if (ch == '0') {
        digits = 1;
        val = 0;

        int next = getchar();
        if (next == 'x' || next == 'X') {
            // 0x prefix recognized
        } else {
            ungetc(next, stdin);
        }
    } else {
        ungetc(ch, stdin);
    }

    // Read remaining hex digits
    while ((ch = getchar()) != EOF) {
        int d;
        if ('0' <= ch && ch <= '9') d = ch - '0';
        else if ('a' <= ch && ch <= 'f') d = ch - 'a' + 10;
        else if ('A' <= ch && ch <= 'F') d = ch - 'A' + 10;
        else {
            ungetc(ch, stdin);
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
            p++;  // move past '%'

            // Handle literal "%%" case: match '%' in input but don't count as assignment
            if (*p == '%') {
                if (!match_literal('%')) goto end; // match input %
                continue; // do NOT increment assigned
            }

            // Parse optional width
            int width = 0;
            while (isdigit(*p)) { width = width * 10 + (*p - '0'); p++; }

            // Parse length modifier (hh, h, l, ll)
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
        } else if (isspace(*p)) {
            skip_whitespace(); // skip spaces in format
        } else if (!match_literal(*p)) {
            goto end; // literal character must match input
        }
    }

end:
    va_end(args);
    return assigned ? assigned : (feof(stdin) ? EOF : 0);
}

/* ===================================
   FULL EXTENDED TEST SUITE
   =================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* =========================
   GLOBAL TEST COUNTERS
   ========================= */
static int tests_run = 0;
static int tests_passed = 0;

/* =========================
   OUTPUT HELPERS
   ========================= */
void print_section(const char *title) {
    printf("\n== %s ==\n", title);
}

void pass(const char *label) {
    printf("  ✓ PASS: %s\n", label);
    tests_passed++;
    tests_run++;
}

void fail(const char *label) {
    printf("  ✗ FAIL: %s\n", label);
    tests_run++;
}

/* =========================
   INPUT REDIRECTION
   ========================= */
void with_input(const char *input, void (*test_fn)(void)) {
    FILE *tmp = fopen("test_input.txt", "w");
    if (!tmp) { perror("fopen"); exit(1); }
    fputs(input, tmp);
    fclose(tmp);

    freopen("test_input.txt", "r", stdin);
    test_fn();
}

/* =========================
   INTEGER %d
   ========================= */
static int scan_val, scan_ret;
static int my_val, my_ret;

void run_scanf_int(void) { scan_val=-999; scan_ret=scanf("%d", &scan_val); }
void run_myscanf_int(void) { my_val=-999; my_ret=my_scanf("%d", &my_val); }

void test_int(const char* label, const char* input) {
    with_input(input, run_scanf_int);
    with_input(input, run_myscanf_int);
    if(scan_ret==my_ret && scan_val==my_val) pass(label);
    else { printf("    scanf=%d val=%d | myscanf=%d val=%d\n", scan_ret, scan_val, my_ret, my_val); fail(label);}
}

void test_integers(void) {
    print_section("Extended integer tests");
    /* normal */
    test_int("positive","42\n");
    test_int("negative","-99\n");
    test_int("explicit +"," +123\n");
    test_int("zero","0\n");
    test_int("leading spaces","   456\n");
    test_int("trailing garbage","789abc\n");
    /* edge */
    test_int("empty input","\n");
    test_int("spaces only","   \n");
    test_int("just minus","-\n");
    test_int("just plus","+ \n");
    test_int("INT_MAX","2147483647\n");
    test_int("INT_MIN","-2147483648\n");
    test_int("overflow","999999999999\n");
    /* width */
    test_int("width 3","12345\n"); // scanf reads first 3 digits
    test_int("width 5 leading zeros","00042\n");
}

/* =========================
   HEX %x
   ========================= */
static unsigned hx1,hx2;
static int hx_r1,hx_r2;
void run_scanf_hex(void){hx1=0; hx_r1=scanf("%x",&hx1);}
void run_myscanf_hex(void){hx2=0; hx_r2=my_scanf("%x",&hx2);}
void test_hex(const char* label,const char* input){
    with_input(input,run_scanf_hex);
    with_input(input,run_myscanf_hex);
    if(hx_r1==hx_r2 && hx1==hx2) pass(label);
    else { printf("    scanf=%d val=%x | myscanf=%d val=%x\n", hx_r1,hx1,hx_r2,hx2); fail(label);}
}
void test_hexes(void){
    print_section("Extended hex tests");
    test_hex("simple hex","ff\n");
    test_hex("uppercase","ABCD\n");
    test_hex("0x prefix","0x1a\n");
    test_hex("0X prefix","0X1A\n");
    test_hex("zero","0\n");
    test_hex("0x only","0x\n");
    test_hex("invalid digit","0xG\n");
    test_hex("leading zeros","000ff\n");
    test_hex("trailing garbage","1fZZ\n");
}

/* =========================
   BINARY %b
   ========================= */
static int bval,bret;
void run_myscanf_bin(void){bval=-1; bret=my_scanf("%b",&bval);}
void test_bin(const char* label,const char* input,int expected){
    with_input(input,run_myscanf_bin);
    if(bret==1 && bval==expected) pass(label);
    else { printf("    got ret=%d val=%d expected=%d\n",bret,bval,expected); fail(label);}
}
void test_binaries(void){
    print_section("Extended binary tests");
    test_bin("101","101\n",5);
    test_bin("0b101","0b101\n",5);
    test_bin("0B111","0B111\n",7);
    test_bin("zero","0\n",0);
    test_bin("invalid stop","102\n",2);
    test_bin("leading spaces","   110\n",6);
    test_bin("long binary","11111111\n",255);
    test_bin("max binary","1111111111111111111111111111111\n",0x7FFFFFFF);
}

/* =========================
   STRINGS %s
   ========================= */
static char s1[128],s2[128]; static int sr1,sr2;
void run_scanf_string(void){memset(s1,0,sizeof(s1)); sr1=scanf("%s",s1);}
void run_myscanf_string(void){memset(s2,0,sizeof(s2)); sr2=my_scanf("%s",s2);}
void test_str(const char* label,const char* input){
    with_input(input,run_scanf_string);
    with_input(input,run_myscanf_string);
    if(sr1==sr2 && strcmp(s1,s2)==0) pass(label);
    else { printf("    scanf=%d val='%s' | myscanf=%d val='%s'\n",sr1,s1,sr2,s2); fail(label);}
}
void test_strings_extended(void){
    print_section("Extended string tests");
    test_str("simple","hello\n");
    test_str("leading spaces","   world\n");
    test_str("stops at space","hi there\n");
    test_str("empty input","\n");
    test_str("only spaces","   \n");
    test_str("punctuation","foo,bar\n");
    test_str("numbers","123abc\n");
    test_str("max length","abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n");
}

/* =========================
   FLOAT %f
   ========================= */
static float f1,f2; static int fr1,fr2;
void run_scanf_float(void){f1=0; fr1=scanf("%f",&f1);}
void run_myscanf_float(void){f2=0; fr2=my_scanf("%f",&f2);}
void test_float(const char* label,const char* input){
    with_input(input,run_scanf_float);
    with_input(input,run_myscanf_float);
    if(fr1==fr2 && f1==f2) pass(label);
    else { printf("    scanf=%d val=%f | myscanf=%d val=%f\n",fr1,f1,fr2,f2); fail(label);}
}
void test_floats_extended(void){
    print_section("Extended float tests");
    test_float("positive","3.14\n");
    test_float("negative","-0.001\n");
    test_float("zero","0.0\n");
    test_float("+0.0","+0.0\n");
    test_float("-0.0","-0.0\n");
    test_float("tiny","1.2e-30\n");
    test_float("huge","1e30\n");
    test_float("scientific","1e2\n");
    test_float("leading spaces","   2.71\n");
    test_float("trailing garbage","1.23abc\n");
}

/* =========================
   MIXED TESTS
   ========================= */
static int mi1,mi2; static unsigned mx1,mx2; static float mf1,mf2; static char ms1[64],ms2[64];
static int mr1,mr2;

void run_scanf_mixed(void){mi1=0; mx1=0; mf1=0; memset(ms1,0,sizeof(ms1)); mr1=scanf("%d %x %f %s",&mi1,&mx1,&mf1,ms1);}
void run_myscanf_mixed(void){mi2=0; mx2=0; mf2=0; memset(ms2,0,sizeof(ms2)); mr2=my_scanf("%d %x %f %s",&mi2,&mx2,&mf2,ms2);}

void test_mixed(const char* label,const char* input){
    with_input(input,run_scanf_mixed);
    with_input(input,run_myscanf_mixed);
    if(mr1==mr2 && mi1==mi2 && mx1==mx2 && mf1==mf2 && strcmp(ms1,ms2)==0) pass(label);
    else { printf("    scanf=%d %d %x %f '%s' | myscanf=%d %d %x %f '%s'\n",mr1,mi1,mx1,mf1,ms1,mr2,mi2,mx2,mf2,ms2); fail(label);}
}

void test_mixed_extended(void){
    print_section("Extended mixed type tests");
    test_mixed("normal","42 ff 3.14 hello\n");
    test_mixed("leading spaces","   7 0x10 2.71 world\n");
    test_mixed("partial second","9 ZZZ 1.23 foo\n");
    test_mixed("zeros","0 0 0.0 zero\n");
    test_mixed("max values","2147483647 ffffffff 1e30 max\n");
}

/* =========================
   MAIN
   ========================= */
int main(void){
    test_integers();
    test_hexes();
    test_binaries();
    test_strings_extended();
    test_floats_extended();
    test_mixed_extended();

    printf("\n====================\n");
    printf("TEST SUMMARY\n");
    printf("Passed %d / %d tests\n",tests_passed,tests_run);
    printf("====================\n");
    return 0;
}
