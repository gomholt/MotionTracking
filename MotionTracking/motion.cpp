#include "motion.hpp"

namespace {

}

Motion::Motion(){

}


int Motion::run() {
	int thresh = 36;
	int thresh_max = 255;

	int box_thresh = 50;
	int box_thresh_max = 255;


	cv::VideoCapture cap(1); // open the default camera
	if (!cap.isOpened())  // check if we succeeded
		return -1;

	bool first = true;

	cv::namedWindow("color", 1);
	cv::namedWindow("binary", 1);
	cv::namedWindow("ROI", 0);
	cv::namedWindow("MasterBackground", 1);

	cv::createTrackbar("Threshold Value", "binary", &thresh, thresh_max);
	cv::createTrackbar("Linethreshold", "color", &box_thresh, box_thresh_max);

	/*cv::createTrackbar("Hue", "ROI", &H_thresh, H_thresh_max);
	cv::createTrackbar("Saturation", "ROI", &S_thresh, S_thresh_max);
	cv::createTrackbar("Value", "ROI", &V_thresh, V_thresh_max);
	cv::createTrackbar("range +-", "ROI", &HSV_range, HSV_range_max);*/

	cap >> prev;
	for (int i = 0; i < 480; i++) {
		mbkg.avgPixelCount[i] = new int[640];
	}
	mbkg.mat = cv::Mat(prev.size(), prev.type());

	for (;;)
	{
		
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

		cv::Mat currOutput;
		currOutput = cv::Mat(curr.size(), curr.type());
		curr.copyTo(currOutput);
		box = calcROIBox(box_thresh, box_thresh_max);


		if (!box.empty()) {
			contourROI(300);
			cv::Rect bound{ box[0],box[2], box[1] - box[0],box[3] - box[2] };
			cv::rectangle(currOutput, bound, CV_RGB(255, 0, 0), 2);
			cv::rectangle(frameDiff.mat, bound, CV_RGB(255, 0, 0), 2);
			std::cout << box[0] <<" "<< box[1]<< " "<<box[2]<< " "<<box[3] << std::endl;
		}
		
		
		
		
		if (!box.empty()) {
			calcAvgBkg(box);
		}
		else {
			std::vector<int> temp = { 0,0,0,0 };
			calcAvgBkg(temp);
		}


		cv::imshow("binary", frameDiff.mat);
		cv::imshow("color", currOutput);
		cv::imshow("MasterBackground", mbkg.mat);

		if (cv::waitKey(5) >= 0) {}
		
	}
	return 0;
}



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


bool Motion::inBox(std::vector<int>box, int x, int y) {
	if ((x > box[0] && x < box[1]) && (y > box[2] && y < box[3])) {
		return true;
	}
	else {
		return false;
	}
}


void Motion::calcAvgBkg(std::vector<int> area) {
	for (int i = 0; i < curr.rows; ++i) {

		cv::Vec3b* currPixel = curr.ptr<cv::Vec3b>(i);
		cv::Vec3b* mbkgPixel = mbkg.mat.ptr<cv::Vec3b>(i);

		for (int j = 0; j < curr.cols; ++j) {
			//std::cout << j << "  " << i << std::endl;
			if (!inBox(area, j, i)) {
				mbkgPixel[j] = { (uchar)(((mbkgPixel[j][0] * mbkg.avgPixelCount[i][j]) + currPixel[j][0]) / (mbkg.avgPixelCount[i][j] + 1)),
					(uchar)(((mbkgPixel[j][1] * mbkg.avgPixelCount[i][j]) + currPixel[j][1]) / (mbkg.avgPixelCount[i][j] + 1)),
					(uchar)(((mbkgPixel[j][2] * mbkg.avgPixelCount[i][j]) + currPixel[j][2]) / (mbkg.avgPixelCount[i][j] + 1)) };
			}
			mbkg.avgPixelCount[i][j]++;
		}
	}
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
		//std::cout << reducedCountours.size()<< std::endl;
		for (int i = 0; i < reducedCountours.size(); ++i) {
			if (reducedCountours[i].first < max / 10) {
				reducedCountours.erase(reducedCountours.begin());
			}
		}
		//std::cout << reducedCountours.size() << "\n\n\n" << std::endl;
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


