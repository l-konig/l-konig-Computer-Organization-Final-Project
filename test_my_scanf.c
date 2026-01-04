#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "my_scanf.h"

/* =========================
   FORWARD DECLARATIONS
   ========================= */
void test_integers(void);
void test_hex(void);
void test_binary(void);
void test_strings(void);
void test_chars(void);
void test_booleans(void);
void test_delimiters(void);
void test_floats(void);
void test_percent(void);
void test_multi_fields(void);
void test_strings_ext(void);
void test_chars_multiple(void);

/* =========================
   GLOBAL TEST COUNTERS
   ========================= */
static int tests_run = 0;
static int tests_passed = 0;

/* =========================
   HELPERS
   ========================= */
void print_section(const char *title) { printf("\n== %s ==\n", title); }
void pass(const char *label) { printf("  ✓ PASS: %s\n", label); tests_passed++; tests_run++; }
void fail(const char *label) { printf("  ✗ FAIL: %s\n", label); tests_run++; }

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
   MY_SCANF DECLARATION
   ========================= */
extern int my_scanf(const char *format, ...);

/* =========================
   INTEGER TESTS %d
   ========================= */
static int scan_ret, scan_val;
static int myscan_ret, myscan_val;

void run_scanf_int(void) { scan_val = -999; scan_ret = scanf("%d", &scan_val); }
void run_myscanf_int(void) { myscan_val = -999; myscan_ret = my_scanf("%d", &myscan_val); }

void test_int_compare(const char *label, const char *input) {
    with_input(input, run_scanf_int);
    with_input(input, run_myscanf_int);
    if (scan_ret == myscan_ret && scan_val == myscan_val) pass(label);
    else {
        printf("    scanf:   ret=%d val=%d\n", scan_ret, scan_val);
        printf("    myscanf: ret=%d val=%d\n", myscan_ret, myscan_val);
        fail(label);
    }
}

void test_integers(void) {
    print_section("Testing integers %d");
    const char *inputs[] = {
        "42\n","-17\n","0\n","   123\n","456abc\n","+99\n","-\n",
        "2147483647\n","-2147483648\n","999999999999999\n","\n","   \n","+0\n","-0\n",
        "00042\n",          // NEW
        "  -0012\n",        // NEW
        "\t77\n"            // NEW
    };
    const char *labels[] = {
        "positive","negative","zero","leading spaces","trailing garbage","explicit plus",
        "just minus","INT_MAX","INT_MIN","overflow","empty input","only spaces",
        "plus zero","minus zero",
        "leading zeros",     // NEW
        "negative leading zeros", // NEW
        "tab leading whitespace"  // NEW
    };
    for(int i=0;i<sizeof(inputs)/sizeof(inputs[0]);i++)
        test_int_compare(labels[i],inputs[i]);
}

/* =========================
   HEX TESTS %x
   ========================= */
static unsigned hx1, hx2;
static int hx_r1, hx_r2;

void run_scanf_hex(void) { hx1 = 0; hx_r1 = scanf("%x", &hx1); }
void run_myscanf_hex(void) { hx2 = 0; hx_r2 = my_scanf("%x", &hx2); }

void test_hex_compare(const char *label, const char *input) {
    with_input(input, run_scanf_hex);
    with_input(input, run_myscanf_hex);
    if (hx_r1 == hx_r2 && hx1 == hx2) pass(label);
    else {
        printf("    scanf:   ret=%d val=%x\n", hx_r1, hx1);
        printf("    myscanf: ret=%d val=%x\n", hx_r2, hx2);
        fail(label);
    }
}

void test_hex(void) {
    print_section("Testing hex %x");
    const char *inputs[] = {
        "ff\n","ABCD\n","0x10\n","0X10\n","0\n","0x\n","0X\n","0xG\n","   1f\n","2Azzz\n","\n"
    };
    const char *labels[] = {
        "simple hex","uppercase","0x prefix","0X prefix","single zero","0x only","0X only","invalid after prefix",
        "leading spaces","trailing garbage","empty input"
    };
    for(int i=0;i<sizeof(inputs)/sizeof(inputs[0]);i++) test_hex_compare(labels[i],inputs[i]);
}

/* =========================
   BINARY TESTS %b
   ========================= */
static int bin_val, bin_ret;

void run_myscanf_bin(void) { bin_val = -1; bin_ret = my_scanf("%b", &bin_val); }

void test_binary_inner(const char *label, const char *input, int expected_val, int expected_ret) {
    with_input(input, run_myscanf_bin);
    if (bin_val == expected_val && bin_ret == expected_ret) pass(label);
    else {
        printf("    input: '%s'\n", input);
        printf("    got ret=%d val=%d  expected ret=%d val=%d\n",
               bin_ret, bin_val, expected_ret, expected_val);
        fail(label);
    }
}

void test_binary(void) {
    print_section("Testing binary %b");
    test_binary_inner("binary 101", "101\n", 5, 1);
    test_binary_inner("binary 0b101", "0b101\n", 5, 1);
    test_binary_inner("binary 0B111", "0B111\n", 7, 1);
    test_binary_inner("binary zero", "0\n", 0, 1);
    test_binary_inner("binary stops at invalid", "102\n", 2, 1);
    test_binary_inner("leading spaces", "   110\n", 6, 1);
    test_binary_inner("empty input", "\n", 0, -1);
    test_binary_inner("only spaces", "   \n", 0, -1);
    test_binary_inner("invalid letters", "abc\n", 0, 0);
    test_binary_inner("digits >1", "456\n", 0, 0);

    test_binary_inner("all zeros", "0000\n", 0, 1);       // NEW
    test_binary_inner("single one", "1\n", 1, 1);         // NEW
    test_binary_inner("space after bits", "101 \n", 5, 1); // NEW
}

/* =========================
   ALL STRING TESTS
   ========================= */
static char s1[128], s2[128];
static int sr1, sr2;
static char s1_ext[128], s2_ext[128];
static int sr1_ext, sr2_ext;

void run_scanf_string(void) { memset(s1,0,sizeof(s1)); sr1=scanf("%s",s1);}
void run_myscanf_string(void) { memset(s2,0,sizeof(s2)); sr2=my_scanf("%s",s2);}
void run_scanf_string_ext(void) { memset(s1_ext,0,sizeof(s1_ext)); sr1_ext=scanf("%s", s1_ext); }
void run_myscanf_string_ext(void) { memset(s2_ext,0,sizeof(s2_ext)); sr2_ext=my_scanf("%s", s2_ext); }

void test_string_compare(const char *label, const char *input) {
    with_input(input, run_scanf_string);
    with_input(input, run_myscanf_string);
    if (sr1==sr2 && strcmp(s1,s2)==0) pass(label);
    else { printf("    scanf: '%s'\n", s1); printf("    myscanf: '%s'\n", s2); fail(label);}
}

void test_string_compare_ext(const char *label, const char *input) {
    with_input(input, run_scanf_string_ext);
    with_input(input, run_myscanf_string_ext);
    if (sr1_ext==sr2_ext && strcmp(s1_ext,s2_ext)==0) pass(label);
    else { printf("    scanf: '%s'\n", s1_ext); printf("    myscanf: '%s'\n", s2_ext); fail(label);}
}

void test_strings(void) {
    print_section("Testing strings %s");
    const char *basic_inputs[] = {"hello\n","   world\n","hi there\n","\n"};
    const char *basic_labels[] = {"simple","leading spaces","stops at space","empty input"};
    for(int i=0;i<4;i++) test_string_compare(basic_labels[i],basic_inputs[i]);

    const char *ext_inputs[] = {"\n","   \n","\t\n","hello world\n","   leading\n","tab\tend\n","  mix\tspace\n","abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n"};
    const char *ext_labels[] = {"empty string","space only","tab only","string with spaces after","string with leading spaces","string with tabs after","string with mixed whitespace","max length string"};
    for(int i=0;i<8;i++) test_string_compare_ext(ext_labels[i],ext_inputs[i]);
}

/* =========================
   ALL CHAR TESTS
   ========================= */
static char c1,c2;
static int cr1, cr2;
static char c_seq1,c_seq2,c_seq3;
static int cr_seq1, cr_seq2;

void run_scanf_char(void){cr1=scanf("%c",&c1);}
void run_myscanf_char(void){cr2=my_scanf("%c",&c2);}
void run_scanf_chars_seq(void){cr_seq1=scanf("%c%c%c",&c_seq1,&c_seq2,&c_seq3);}
void run_myscanf_chars_seq(void){cr_seq2=my_scanf("%c%c%c",&c_seq1,&c_seq2,&c_seq3);}

void test_char_compare(const char *label,const char *input){
    with_input(input, run_scanf_char);
    with_input(input, run_myscanf_char);
    if(cr1==cr2 && c1==c2) pass(label);
    else{printf("    scanf: '%c'  myscanf: '%c'\n",c1,c2);fail(label);}
}

void test_chars_seq(const char *label,const char *input){
    with_input(input, run_scanf_chars_seq);
    with_input(input, run_myscanf_chars_seq);
    if(cr_seq1==cr_seq2) pass(label);
    else{printf("    scanf: '%c%c%c'  myscanf: '%c%c%c'\n",c_seq1,c_seq2,c_seq3,c_seq1,c_seq2,c_seq3);fail(label);}
}

void test_chars(void){
    print_section("Testing chars %c");
    test_char_compare("visible char","A\n");
    test_char_compare("space char"," \n");
    test_char_compare("newline char","\n");
    test_char_compare("tab char","\t");
    test_char_compare("empty input","\n");
}

void test_chars_multiple(void){
    print_section("Testing multiple chars %c%c%c");
    test_chars_seq("ABC sequence","ABC\n");
    test_chars_seq("digits","123\n");
    test_chars_seq("mixed chars","A1b\n");
    test_chars_seq("spaces and tabs"," \tX\n");
    test_chars_seq("newlines","\n\n\n");
    test_chars_seq("empty input","\n");
}

/* =========================
   BOOLEAN TESTS %B
   ========================= */
static int bool_val,bool_ret;
void run_myscanf_bool(void){bool_val=-1; bool_ret=my_scanf("%B",&bool_val);}
void test_boolean(const char *label,const char *input,int expected_val){
    with_input(input,run_myscanf_bool);
    if(bool_val==expected_val) pass(label);
    else{printf("    input: '%s' got %d expected %d\n",input,bool_val,expected_val); fail(label);}
}

void test_booleans(void){
    print_section("Testing booleans %B");
    test_boolean("true","true\n",1);
    test_boolean("false","false\n",0);
    test_boolean("1","1\n",1);
    test_boolean("0","0\n",0);
    test_boolean("uppercase TRUE","TRUE\n",1);
    test_boolean("uppercase FALSE","FALSE\n",0);
    test_boolean("leading space","   true\n",1);
    test_boolean("empty input","\n",0);
    test_boolean("invalid letters","abc\n",0);
    test_boolean("mixed-case TrUe","TrUe\n",1);
    test_boolean("mixed-case fAlSe","fAlSe\n",0);
}

/* =========================
   DELIMITER TESTS %D
   ========================= */
static char delim_val[64];
void run_myscanf_delim(void){memset(delim_val,0,sizeof(delim_val)); my_scanf("%D,",delim_val);}
void test_delimiter(const char *label,const char *input,const char *expected){
    with_input(input,run_myscanf_delim);
    if(strcmp(delim_val,expected)==0) pass(label);
    else{printf("    input: '%s' got '%s' expected '%s'\n",input,delim_val,expected); fail(label);}
}
void test_delimiters(void){
    print_section("Testing delimiters %D");
    test_delimiter("comma","hello,world\n","hello");
    test_delimiter("space","foo bar\n","foo");
    test_delimiter("tab","a\tb\n","a");
    test_delimiter("newline","x\ny\n","x");
    test_delimiter("delimiter at end","abc,\n","abc");
    test_delimiter("empty input","\n","");
    test_delimiter("multiple delimiters","a,b,c\n","a");
    test_delimiter("delimiter first",",abc\n","");
    test_delimiter("no delimiter","abc\n","abc");
}

/* =========================
   FLOAT TESTS %f
   ========================= */
static double d1; static int dr1;
static float f2; static int fr2;
void run_scanf_double(void){d1=0; dr1=scanf("%lf",&d1);}
void run_myscanf_float(void){f2=0; fr2=my_scanf("%f",&f2);}
void test_float_compare(const char *label,const char *input){
    with_input(input,run_scanf_double);
    with_input(input,run_myscanf_float);
    if(dr1==fr2 && fabs(d1-f2)<1e-6) pass(label);
    else{printf("    scanf: %g ret=%d\n",d1,dr1); printf("    myscanf: %g ret=%d\n",(double)f2,fr2); fail(label);}
}
void test_floats(void){
    print_section("Testing floats %f");
    const char *inputs[]={"3.14\n","-2.718\n","0\n","   1.23\n","4.56abc\n",".\n","1e3\n","-2.5E-2\n","\n","   \n"};
    const char *labels[]={"simple","negative","zero","leading spaces","trailing garbage","only dot","scientific","negative scientific","empty input","only spaces"};
    for(int i=0;i<10;i++) test_float_compare(labels[i],inputs[i]);
}

/* =========================
   PERCENT LITERAL %%
   ========================= */
static int perc_scanf_ret,perc_myscanf_ret;
void run_scanf_percent(void){perc_scanf_ret=scanf("%%");}
void run_myscanf_percent(void){perc_myscanf_ret=my_scanf("%%");}
void test_percent(void){
    print_section("Testing percent literal %%");
    const char *inputs[]={"%\n","%%\n","\n","abc\n","%%%%\n","%x\n"};
    for(int i=0;i<6;i++){
        with_input(inputs[i],run_scanf_percent);
        with_input(inputs[i],run_myscanf_percent);
        int ok=inputs[i][0]=='%'? (perc_scanf_ret==perc_myscanf_ret):(perc_scanf_ret<=0 && perc_myscanf_ret<=0);
        if(ok) pass(inputs[i]); else{printf("    input: '%s' scanf=%d myscanf=%d\n",inputs[i],perc_scanf_ret,perc_myscanf_ret); fail(inputs[i]);}
    }
}

/* =========================
   MULTI-FIELD TESTS
   ========================= */
static int mf_d; static unsigned mf_x; static float mf_f; static char mf_s[128]; static char mf_c1,mf_c2;
static int mf_ret_scanf, mf_ret_myscanf;
void run_scanf_multi(void){mf_d=-999; mf_x=0; mf_f=0; mf_s[0]='\0'; mf_c1=mf_c2=0; mf_ret_scanf=scanf("%d %x %f %s %c %c",&mf_d,&mf_x,&mf_f,mf_s,&mf_c1,&mf_c2);}
void run_myscanf_multi(void){mf_d=-999; mf_x=0; mf_f=0; mf_s[0]='\0'; mf_c1=mf_c2=0; mf_ret_myscanf=my_scanf("%d %x %f %s %c %c",&mf_d,&mf_x,&mf_f,mf_s,&mf_c1,&mf_c2);}
void test_multi_compare(const char *label,const char *input){
    with_input(input,run_scanf_multi);
    with_input(input,run_myscanf_multi);
    if(mf_ret_scanf==mf_ret_myscanf && fabs(mf_f-mf_f)<1e-6) pass(label);
    else{printf("    input: '%s' scanf_ret=%d myscanf_ret=%d\n",input,mf_ret_scanf,mf_ret_myscanf); fail(label);}
}
void test_multi_fields(void){
    print_section("Testing multiple fields together");
    const char *inputs[]={
        "42 ff 3.14 hello A B\n",
        "   7 1a 2.718 world X Y\n",
        "123 0F 0.5 test1 Z K garbage\n",
        "1 1 1.0 a a b\n",
        "10 10 1e3 sci E F\n",
        "abc 1 0.1 str M N\n",
        "5 5 5.5 five F G\n",
        "0 0 0.0 zero Z Z\n",
        "9 9 9.9 nine N N\n"
    };
    const char *labels[]={
        "simple mix","leading spaces","trailing garbage","minimal spacing",
        "scientific float","invalid int","clean words","all zeros","repeated fields"
    };
    for(int i=0;i<9;i++)
        test_multi_compare(labels[i],inputs[i]);
}

/* =========================
   MAIN
   ========================= */
int main(void){
    test_integers();
    test_hex();
    test_binary();
    test_strings();
    test_chars();
    test_chars_multiple();
    test_booleans();
    test_delimiters();
    test_floats();
    test_percent();
    test_multi_fields();
    printf("\nTests passed %d/%d\n",tests_passed,tests_run);
    return 0;
}
