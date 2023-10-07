/*
 * MIT License
 * 
 * Copyright (c) 2023 Can Joshua Lehmann
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

__asm__(
  ".globl _start\n"
  "_start: call main\n"
  "ebreak"
);

void print_char(const char chr) {
  __asm__(
    "addi a0, zero, 0\n"
    "addi a1, %0, 0\n"
    "ecall"
    :
    : "r" (chr)
    : "a0", "a1"
  );
}

void dbgi(int value) {
  __asm__(
    "addi a0, zero, 1\n"
    "addi a1, %0, 0\n"
    "ecall"
    :
    : "r" (value)
    : "a0", "a1"
  );
}

void print_string(const char* str) {
  for (const char* cur = str; *cur != 0; cur++) {
    print_char(*cur);
  }
}

unsigned bitwidth(unsigned x) {
  for (unsigned it = sizeof(x) * 8; it-- > 0;) {
    if (x & (1 << it)) {
      return it + 1;
    }
  }
  return 0;
}

void divmod(unsigned a, unsigned b, unsigned* div, unsigned* mod) {
  *div = 0;
  *mod = a;
  for (unsigned it = sizeof(a) * 8 - bitwidth(b) + 1; it-- > 0;) {
    if ((b << it) <= *mod) {
      *div |= 1 << it;
      *mod -= b << it;
    }
  }
}

void print_int(int value) {
  #define BUFFER_SIZE 16
  char buffer[BUFFER_SIZE] = {0};
  int index = BUFFER_SIZE - 2;
  unsigned cur = value > 0 ? value : -value;
  while (cur != 0 || index == BUFFER_SIZE - 2) {
    unsigned mod;
    divmod(cur, 10, &cur, &mod);
    buffer[index] += '0' + mod;
    index--;
  }
  if (value < 0) {
    buffer[index] = '-';
    index--;
  }
  print_string(buffer + index + 1);
  #undef BUFFER_SIZE
}

int main() {
  int a = 0;
  int b = 1;
  for (int it = 0; it < 20; it++) {
    print_int(a);
    print_char('\n');
    
    int temp = b;
    b += a;
    a = temp;
  }
  return 0;
}
