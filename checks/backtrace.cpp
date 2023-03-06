#include <cstddef>
#include <execinfo.h>
int main(void) {
	// DO NOT alloc memory on heap!!
	void *array[25];
	const size_t size = backtrace(array, 25);
	return 0;
}
