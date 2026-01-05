# Leora Konig
# my_scanf Project

## Project Description
This project implements a custom version of C's `scanf` function called `my_scanf`, which mimics the behavior of the standard C `scanf` but also adds 3 custom extensions:

### 1. Boolean Extension (`%b`)  
Reads boolean values: `true/false`, `yes/no`, `on/off`, or `1/0` into an integer (`1 = true`, `0 = false`).  
**Returns:**
- `1` if a valid boolean was read  
- `0` if the input was invalid

---

### 2. Binary Extension (`%B`)  
Reads binary numbers, optionally starting with `0b` or `0B`, and converts them to decimal.  
**Returns:**
- `1` if at least one valid binary digit was read  
- `0` if the input was invalid (letters, empty, only prefix)  
- `-1` if EOF is reached before reading any digit  

---

### 3. Delimiter Extension (`%D`)  
Reads a string from standard input into a buffer until one of the following occurs:
- The specified delimiter (can be one or more characters) is matched  
- A newline, space, or tab is encountered (only for single-character delimiters)  
- The maximum width of the buffer is reached  
- EOF occurs

**Behavior:**
- Returns `1` if at least one character is read  
- Returns `0` if nothing is read before the delimiter  
- Returns `-1` if EOF is reached before reading any character  
- Spaces and other characters **before multi-character delimiters are preserved**  
- Trailing newline is removed if no delimiter is matched  

---

## Tests
The `test_my_scanf.c` file contains extensive tests for:
- Standard `scanf` behavior (integers, strings, etc.)  
- Boolean, binary, and delimiter extensions  
- Edge cases for all functions
All tests are designed to compare `my_scanf` output with standard `scanf` where applicable, and with the expected behavior for the custom extensions.

---

## How to Run on ADA
1. **Log in to ADA**
2. **Clone the Repository**

  git clone https://github.com/l-konig/l-konig-Computer-Organization-Final-Project.git

  cd l-konig-Computer-Organization-Final-Project

3. **Compile the Code**

  gcc test_my_scanf.c my_scanf.c -lm -o test_my_scanf

  --> The -lm flag links the math library which provides functions like pow(), fabs(), etc. Needed because my_scanf.c uses pow() when parsing floating-point numbers with exponents.

4. **Run the Tests**

  ./test_my_scanf
