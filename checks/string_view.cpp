#include <string>
#include <string_view>
int main(void) {
	std::string foo("hello");
	std::string_view sv{foo};
	return foo == sv;
}
