#include <math.h>

int position_of_set_bit(int n) {
  return (n & (n - 1)) == 0 ? ((int)log2(n)) : -1;
}
