int main() {
  int a = 1;
  int b = 7;
  for (int i = 0; i < 100; ++i) {
    if (a == 1) {
      b = 6;
    }
    if (a == 3) {
      b = 7;
    }
    if (b == 6) {
      a = 3;
    }
  }
}