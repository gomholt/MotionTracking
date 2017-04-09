#include "cvGoodies.hpp"

namespace {

}

cvGoodies::cvGoodies()
{
	
}

void cvGoodies::run() {
	std::cout << "Select Computer Vision app to demo:" << std::endl;
	std::cout << "\t1)basic motion detection" << std::endl;
	std::cout << "\t2)smart background" << std::endl;
	std::cout << "\tmore soon..." << std::endl;
	int option = 0;
	do {
		std::cin >> option;
		
	} while (option < 1 || option > 2);

	std::cout << option << std::endl;
	switch (option) {
	case 1:
	{
		Motion app;
		app.run();
		break; }
	case 2:
		smartBackground app;
		app.run();
		break;
	}

}
