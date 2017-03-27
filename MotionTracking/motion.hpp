#ifndef _motion
#define _motion
#include "opencv2/opencv.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\imgproc.hpp"
#include <iostream>
#include <queue>
#include <algorithm>

class Motion 
{
private:
	struct diff {
		cv::Mat mat;
		int* rowDifftotals = new int[480];
		int* colDifftotals = new int[640];
		int rowDiffmax = 0;
		int colDiffmax = 0;
	};

	struct background {
		cv::Mat mat;
		int** avgPixelCount = new int*[480];
	};

	int mode = 0;		//what mode of motion to run
	cv::Mat curr;		
	cv::Mat prev;
	diff frameDiff;
	background mbkg;			//master background
	std::vector<int> box = { 0,0,0,0 };
	bool steady;

	void filterHSV(int H_thresh, int HSV_range, int V_thresh);
	void calcDiffRGB(int thresh);

	void initBkg(std::vector<int> box);
	int calcAvgBkg(std::vector<int>box, int thresh);
	void contourROI(int threshHold);

	void concatMats(cv::Mat& first, cv::Mat& second);


	std::vector<int> calcROIBox(int box_thresh, int box_thresh_max);
	bool inBox(std::vector<int>box, int x, int y);

public:
	Motion();
	int run();			//executes the motion process


};

#endif