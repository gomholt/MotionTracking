#ifndef _smartBackground
#define _smartBackground
#include "motion.hpp"

class smartBackground : public Motion {
private:

	struct background {
		cv::Mat mat;
		int** avgPixelCount = new int*[480];
		int countMax = 0;
	};

	background mbkg;
	bool first;
	int thresh;

	void initBkg(std::vector<int> box);
	int calcAvgBkg(std::vector<int>box, int thresh);
	bool inBox(std::vector<int>box, int x, int y);

	int initBkg();
	void loopBkgBody();

public:
	smartBackground();
	void run();

};

#endif