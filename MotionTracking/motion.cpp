#include "motion.hpp"

namespace {

}

Motion::Motion(){
	thresh = 36;
	thresh_max = 255;

	box_thresh = 50;
	box_thresh_max = 255;

	
	
}

int Motion::initMotion() {
	cap = cv::VideoCapture(1); // open the default camera
	if (!cap.isOpened()) {  // check if we succeeded
		return -1;
	}

	cap >> prev;


	for (int i = 0; i < 4; i++) {
		recentBoxQ.push_back(std::queue<int>());
		recentBoxS.push_back(std::multiset<int>());
		recentMax.push_back(0);
	}
}


int Motion::run() {
	cv::namedWindow("color", 1);
	cv::namedWindow("binary", 1);

	cv::createTrackbar("Threshold Value", "binary", &thresh, thresh_max);
	cv::createTrackbar("Linethreshold", "color", &box_thresh, box_thresh_max);

	initMotion();
	
	for (;;)
	{
		loopMotionBody();
		
		cv::imshow("binary", frameDiff.mat);
		cv::imshow("color", currOutput);

		if (cv::waitKey(5) >= 0) {}
		
	}
	return 0;
}

void Motion::loopMotionBody() {
	cap >> curr;

	frameDiff.colDiffmax = 0;
	frameDiff.rowDiffmax = 0;
	for (int i = 0; i < 480; i++) {
		frameDiff.rowDifftotals[i] = 0;
	}
	for (int i = 0; i < 640; i++) {
		frameDiff.colDifftotals[i] = 0;
	}


	box = { 0,0,0,0 };
	frameDiff.mat = cv::Mat(curr.size(), curr.type());
	cv::GaussianBlur(curr, curr, cv::Size(7, 7), 1.5, 1.5);

	calcDiffRGB(thresh);
	curr.copyTo(prev);

	currOutput = cv::Mat(curr.size(), curr.type());
	curr.copyTo(currOutput);
	box = calcROIBox(box_thresh, box_thresh_max);


	if (!box.empty()) {
		contourROI(300);
		cv::Rect bound{ box[0],box[2], box[1] - box[0],box[3] - box[2] };
		cv::rectangle(currOutput, bound, CV_RGB(255, 0, 0), 2);
		cv::rectangle(frameDiff.mat, bound, CV_RGB(255, 0, 0), 2);
		updateRecentBox(box);
	}

}


void Motion::updateRecentBox(std::vector<int> area) {
	recentMax = { 0,0,0,0 };
	for (int i = 0; i < 4; i++) {
		if (recentBoxQ[i].size() > 10) {
			recentBoxS[i].erase(recentBoxQ[i].front());
			recentBoxQ[i].pop();
		}
		recentBoxQ[i].push(area[i]);
		recentBoxS[i].insert(area[i]);
	}

	for (int i = 0; i < 4; i++) {
		if (i == 1 || i == 3) {
			recentMax[i] = (double)*(recentBoxS[i].rbegin()) * 1.05;
		}
		else {
			recentMax[i] = (double)*(recentBoxS[i].begin())* .95;
		}	
	}	
}

/*
not in use currently
*/
void Motion::filterHSV(int H_thresh, int HSV_range, int V_thresh) {
	for (int i = 0; i < prev.rows; ++i) {
		cv::Vec3b* currPixel = curr.ptr<cv::Vec3b>(i);
		cv::Vec3b* prevPixel = prev.ptr<cv::Vec3b>(i);
		cv::Vec3b* diffPixel = frameDiff.mat.ptr<cv::Vec3b>(i);

		for (int j = 0; j < prev.cols; ++j) {
			if ((currPixel[j][0] >(H_thresh - HSV_range) && currPixel[j][0] < (H_thresh + HSV_range))
				/*&& (currPixel[j][1] > (S_thresh - HSV_range) && currPixel[j][1] < (S_thresh + HSV_range))*/
				&& (currPixel[j][2] > (V_thresh - HSV_range) && currPixel[j][2] < (V_thresh + HSV_range))) {

				diffPixel[j] = { currPixel[j][0], currPixel[j][1], currPixel[j][2] };
			}
			else {
				diffPixel[j] = { 0,0,0 };
			}
		}
	}
}


void Motion::calcDiffRGB(int thresh) {


	for (int i = 0; i < prev.rows; ++i) {
		cv::Vec3b* currPixel = curr.ptr<cv::Vec3b>(i);
		cv::Vec3b* prevPixel = prev.ptr<cv::Vec3b>(i);
		cv::Vec3b* diffPixel = frameDiff.mat.ptr<cv::Vec3b>(i);
		

		for (int j = 0; j < prev.cols; ++j) {
			diffPixel[j][0] = (currPixel[j][0] > prevPixel[j][0]) ? currPixel[j][0] - prevPixel[j][0] : prevPixel[j][0] - currPixel[j][0];
			diffPixel[j][1] = (currPixel[j][1] > prevPixel[j][1]) ? currPixel[j][1] - prevPixel[j][1] : prevPixel[j][1] - currPixel[j][1];
			diffPixel[j][2] = (currPixel[j][2] > prevPixel[j][2]) ? currPixel[j][2] - prevPixel[j][2] : prevPixel[j][2] - currPixel[j][2];
			
			if ((diffPixel[j][0] + diffPixel[j][1] + diffPixel[j][2]) > thresh) {
				diffPixel[j] = { 255,255,255 };
				frameDiff.colDifftotals[j] += 1;
				frameDiff.rowDifftotals[i] += 1;
			}
			else {
				diffPixel[j] = { 0,0,0 };
			}
		}
	}

	for (int i = 0; i < prev.rows; ++i) {
		if (frameDiff.rowDifftotals[i] > frameDiff.rowDiffmax) {
			frameDiff.rowDiffmax = frameDiff.rowDifftotals[i];
		}
	}
	for (int i = 0; i < prev.cols; ++i) {
		if (frameDiff.colDifftotals[i] > frameDiff.colDiffmax) {
			frameDiff.colDiffmax = frameDiff.colDifftotals[i];
		}
	}
}


std::vector<int> Motion::calcROIBox(int box_thresh,int box_thresh_max) {
	if (frameDiff.rowDiffmax < 20 && frameDiff.colDiffmax < 20) {
		return{};
	}
	int left, right, top, bottom;
	for (int i = 0; i < 480; ++i) {
		if (frameDiff.rowDifftotals[i] >(double) (frameDiff.rowDiffmax * (double)(box_thresh / box_thresh_max))) {
			top = i;
			break;
		}
	}
	for (int i = 479; i >= 0; --i) {
		if (frameDiff.rowDifftotals[i] > (double)(frameDiff.rowDiffmax * (double)(box_thresh / box_thresh_max))) {
			bottom = i;
			break;
		}
	}
	for (int i = 0; i < 640; ++i) {
		if (frameDiff.colDifftotals[i] >(double) (frameDiff.colDiffmax * (double)(box_thresh / box_thresh_max))) {
			left = i;
			break;
		}
	}
	for (int i = 639; i >= 0; --i) {
		if (frameDiff.colDifftotals[i] > (double)(frameDiff.colDiffmax * (double)(box_thresh / box_thresh_max))) {
			right = i;
			break;
		}
	}
	return{ left,right,top,bottom };
}


void Motion::contourROI(int threshHold) {
	cv::RNG rng(12345);

	cv::Rect bound{ box[0],box[2], box[1] - box[0],box[3] - box[2] };
	if (bound.area() > 300) {
		cv::Mat roiBinary = frameDiff.mat(bound);
		cv::Mat binary;

		cv::cvtColor(roiBinary, binary, CV_BGR2GRAY);
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;
		cv::findContours(binary, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));


		auto order = [](std::pair<double, std::vector<cv::Point>>& a, std::pair<double, std::vector<cv::Point>>&  b) -> bool {
			return a.first < b.first;
		};
		//std::priority_queue<std::pair<double, std::vector<cv::Point>>,std::vector<std::pair<double, std::vector<cv::Point>>>, decltype(order)> reducedContours(order);
		std::vector <std::pair<double, std::vector<cv::Point>>> reducedCountours;

		int count = 0;
		double max = 0;
		for (auto i = contours.begin(); i != contours.end(); ++i) {
			double temp = cv::contourArea(*i);
			if (temp > max) {
				max = temp;
			}
			reducedCountours.push_back(std::make_pair(temp, *i));
		}
		std::sort(reducedCountours.begin(), reducedCountours.end(), order);
		for (int i = 0; i < reducedCountours.size(); ++i) {
			if (reducedCountours[i].first < max / 10) {
				reducedCountours.erase(reducedCountours.begin());
			}
		}
		std::vector<std::vector<cv::Point>> new_contours;
		for (auto i = reducedCountours.begin(); i != reducedCountours.end(); ++i) {
			new_contours.push_back(i->second);
		}



		for (int i = 0; i < new_contours.size(); ++i) {
			cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			cv::drawContours(roiBinary, new_contours, i, color, 2, 8, hierarchy, 0, cv::Point());
		}
	
		cv::Mat roiColor = curr(bound);

		cv::Mat bufferH = cv::Mat(roiColor.rows, (curr.cols * 2 - roiColor.cols * 2), curr.type(), cv::Scalar(50, 50, 50));
		std::vector<cv::Mat> matrices = { roiColor,roiBinary,bufferH };
		cv::hconcat(matrices, bufferH);
		cv::Mat roiScaled = cv::Mat((curr.rows - bufferH.rows), bufferH.cols, curr.type(), cv::Scalar(50, 50, 50));
		cv::vconcat(bufferH, roiScaled, roiScaled);
		roiScaled.cols = curr.cols * 2;
		//cv::imshow("ROI", roiScaled);
	
	}	
}


