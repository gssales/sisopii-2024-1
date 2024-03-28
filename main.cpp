#include <iostream>

int main(int argc, const char *argv[]) {
	
	std::cout << "Hello World" << std::endl;

	if (argc > 1) {
		std::cout << "I am a " << argv[1] << std::endl;
	}

	return 0;
}
