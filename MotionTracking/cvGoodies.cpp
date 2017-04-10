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

	switch (option) {
	case 1:
	{
		std::cout << "Starting Motion App" << std::endl;
		Motion app;
		std::cout << "running Motion App" << std::endl;
		app.run();
		break; }
	case 2:
		smartBackground app;
		app.run();
		break;
	}

}


