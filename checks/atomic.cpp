#include <atomic>
int main(void) {
  std::atomic<double> tmpDouble(0.0);
  std::atomic_bool tmpBool;
  const double d = tmpDouble.load();
  tmpBool = false;
  return 0;
}
