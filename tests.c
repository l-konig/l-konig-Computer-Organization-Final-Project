#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Assuming your fixed my_scanf and scan_string are already included above */

void test_my_scanf_string(const char *input, int expected_return, const char *expected_str) {
    // Feed input into stdin using a temporary file
    FILE *tmp = tmpfile();
    if (!tmp) {
        perror("tmpfile");
        return;
    }

    fprintf(tmp, "%s", input);  // write test input
    rewind(tmp);                // rewind to start for reading

    // Redirect stdin to our tmp file
    FILE *old_stdin = stdin;
    stdin = tmp;

    char buf[256] = {0};
    int ret = my_scanf("%s", buf);

    // Restore stdin
    stdin = old_stdin;

    // Check results
    printf("=== Input: \"%s\" ===\n", input);
    printf("my_scanf returned: %d\n", ret);
    printf("String read: \"%s\"\n", buf);
    if (ret == expected_return && strcmp(buf, expected_str) == 0) {
        printf("✓ Test PASSED\n\n");
    } else {
        printf("✗ Test FAILED (expected return=%d, string=\"%s\")\n\n",
               expected_return, expected_str);
    }

    fclose(tmp);
}

int main() {
    // Normal word
    test_my_scanf_string("hello", 1, "hello");

    // Leading spaces
    test_my_scanf_string("   hello", 1, "hello");

    // Multiple words
    test_my_scanf_string("hello world", 1, "hello");

    // Leading tabs/newlines
    test_my_scanf_string("\t\nhello", 1, "hello");

    // Numbers and letters
    test_my_scanf_string("123abc", 1, "123abc");

    // Non-alphanumeric characters
    test_my_scanf_string("!@#", 1, "!@#");

    // Only Enter (empty input)
    test_my_scanf_string("\n", -1, "");

    // Only spaces
    test_my_scanf_string("      ", -1, "");

    // Word with trailing spaces
    test_my_scanf_string(" word ", 1, "word");

    // Single character
    test_my_scanf_string("a", 1, "a");

    // Multiple single chars separated by spaces
    test_my_scanf_string("a b c", 1, "a");

    // Only whitespace characters
    test_my_scanf_string(" \t \n ", -1, "");

    return 0;
}
