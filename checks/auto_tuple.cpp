#include <tuple>
int main(void) {
  std::tuple<char, int> foo(0,'h');
  auto [c,i] = foo;
  return 0;
}
