#ifndef _motion
#define _motion
#include "opencv2/opencv.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\imgproc.hpp"
#include <iostream>
#include <queue>
#include <set>
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

	int thresh;
	int thresh_max;
	int box_thresh;
	int box_thresh_max;
	bool first;

	cv::VideoCapture cap;

	

	int mode = 0;		//what mode of motion to run
	
	std::vector<std::queue<int>> recentBoxQ;
	std::vector<std::multiset<int>> recentBoxS;
	bool steady;

	void updateRecentBox(std::vector<int> box);

	void filterHSV(int H_thresh, int HSV_range, int V_thresh);
	void calcDiffRGB(int thresh);

	void contourROI(int threshHold);
	
	std::vector<int> calcROIBox(int box_thresh, int box_thresh_max);

public:
	Motion();
	int run();			//executes the motion process

protected:
	int initMotion();
	void loopMotionBody();
	cv::Mat curr;
	cv::Mat prev;
	cv::Mat currOutput;
	diff frameDiff;
	std::vector<int> box = { 0,0,0,0 };
	std::vector<int> recentMax;


};

#endif