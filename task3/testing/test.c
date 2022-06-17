#include <stdio.h>

int main() {
  int dummy = 19260817;
  int jmp = 0;
  int cnt = 0;
  for (int i = 0; i < 1000; ++i) {
    jmp = cnt > 1;
    if (jmp) {
      dummy += 1;
    }
    cnt = (cnt + 1) % 4;
  }
}