#include "smartBackground.hpp"

namespace{

}

smartBackground::smartBackground() {
	first = true;
	thresh = 36;
}

int smartBackground::initBkg() {
	initMotion();
	cv::namedWindow("color", 1);
	cv::namedWindow("MasterBackground", 1);
	
	for (int i = 0; i < 480; i++) {
		mbkg.avgPixelCount[i] = new int[640];
	}
	mbkg.mat = cv::Mat(prev.size(), prev.type());

	int mbkgChange = 0;
	return 0;
}

void smartBackground::run() {

	initBkg();
	
	for (;;) {
		loopMotionBody();
		loopBkgBody();
		cv::imshow("color", currOutput);
		cv::imshow("MasterBackground", mbkg.mat);

		if (cv::waitKey(5) >= 0) {}
	}
}

void smartBackground::loopBkgBody() {
	
	if (first) {
		first = false;
		if (!box.empty()) {
			initBkg(box);
		}
		else {
			std::vector<int> temp = { 0,0,0,0 };
			initBkg(temp);
		}
	}
	else {
		cv::Rect rbound{ recentMax[0],recentMax[2], recentMax[1] - recentMax[0],recentMax[3] - recentMax[2] };
		cv::rectangle(currOutput, rbound, CV_RGB(255, 255, 0), 2);
		if (!recentMax.empty()) {
			calcAvgBkg(recentMax, 100);
		}
		else {
			std::vector<int> temp = { 0,0,0,0 };
			calcAvgBkg(temp, thresh);

		}
	}

}

void smartBackground::initBkg(std::vector<int> box) {
	for (int i = 0; i < curr.rows; ++i) {

		cv::Vec3b* currPixel = curr.ptr<cv::Vec3b>(i);
		cv::Vec3b* mbkgPixel = mbkg.mat.ptr<cv::Vec3b>(i);

		for (int j = 0; j < curr.cols; ++j) {
			if (!inBox(box, j, i)) {
				mbkgPixel[j] = { (uchar)currPixel[j][0], (uchar)currPixel[j][1], (uchar)currPixel[j][2] };
				mbkg.avgPixelCount[i][j] = 1;
			}
			else {
				mbkgPixel[j] = { (uchar)128,(uchar)128, (uchar)128 };
				mbkg.avgPixelCount[i][j] = -1;
			}
		}
	}

}

bool smartBackground::inBox(std::vector<int>box, int x, int y) {
	if ((x > box[0] && x < box[1]) && (y > box[2] && y < box[3])) {
		return true;
	}
	else {
		return false;
	}
}


int smartBackground::calcAvgBkg(std::vector<int> area, int thresh) {
	int totalDiff = 0;
	mbkg.countMax++;
	for (int i = 0; i < curr.rows; ++i) {

		cv::Vec3b* currPixel = curr.ptr<cv::Vec3b>(i);
		cv::Vec3b* mbkgPixel = mbkg.mat.ptr<cv::Vec3b>(i);

		for (int j = 0; j < curr.cols; ++j) {

			if (!inBox(area, j, i)) {
				cv::Vec3b diffTemp;
				diffTemp[0] = (currPixel[j][0] > mbkgPixel[j][0]) ? currPixel[j][0] - mbkgPixel[j][0] : mbkgPixel[j][0] - currPixel[j][0];
				diffTemp[1] = (currPixel[j][1] > mbkgPixel[j][1]) ? currPixel[j][1] - mbkgPixel[j][1] : mbkgPixel[j][1] - currPixel[j][1];
				diffTemp[2] = (currPixel[j][2] > mbkgPixel[j][2]) ? currPixel[j][2] - mbkgPixel[j][2] : mbkgPixel[j][2] - currPixel[j][2];

				if (diffTemp[0] + diffTemp[1] + diffTemp[2] < 300) {
					if (mbkg.avgPixelCount[i][j] == -1) {
						mbkgPixel[j] = { (uchar)currPixel[j][0], (uchar)currPixel[j][1], (uchar)currPixel[j][2] };
						mbkg.avgPixelCount[i][j] = 1;
					}
					else {
						mbkgPixel[j] = { (uchar)(((mbkgPixel[j][0] * mbkg.avgPixelCount[i][j]) + currPixel[j][0]) / (mbkg.avgPixelCount[i][j] + 1)),
							(uchar)(((mbkgPixel[j][1] * mbkg.avgPixelCount[i][j]) + currPixel[j][1]) / (mbkg.avgPixelCount[i][j] + 1)),
							(uchar)(((mbkgPixel[j][2] * mbkg.avgPixelCount[i][j]) + currPixel[j][2]) / (mbkg.avgPixelCount[i][j] + 1)) };
						mbkg.avgPixelCount[i][j]++;

					}
				}
				else {
					if ((double)mbkg.avgPixelCount[i][j] < ((double)mbkg.countMax * 0.5)) {
						mbkgPixel[j] = { (uchar)currPixel[j][0], (uchar)currPixel[j][1], (uchar)currPixel[j][2] };
						mbkg.avgPixelCount[i][j] = 1;
					}
				}
			}

		}
	}
	if (totalDiff > 300000) {
		return 1;
	}
	return totalDiff;
}
