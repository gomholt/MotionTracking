#include "opencv2/opencv.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\imgproc.hpp"
#include <iostream>
#include <queue>
#include <algorithm>

int thresh_max = 100;
int thresh;
int box_thresh_max = 100;
int box_thresh;
cv::RNG rng(12345);

struct diff {
	cv::Mat mat;
	int rowDifftotals[480] = { 0 };
	int colDifftotals[640] = { 0 };
	int rowDiffmax = 0;
	int colDiffmax = 0;
};

void calcDiff(cv::Mat& prev, cv::Mat& curr, diff& diff) {

	for (int i = 0; i < prev.rows; ++i) {
		cv::Vec3b* currPixel = curr.ptr<cv::Vec3b>(i);
		cv::Vec3b* prevPixel = prev.ptr<cv::Vec3b>(i);
		cv::Vec3b* diffPixel = diff.mat.ptr<cv::Vec3b>(i);

		for (int j = 0; j < prev.cols; ++j) {
			diffPixel[j][0] = (currPixel[j][0] > prevPixel[j][0]) ? currPixel[j][0] - prevPixel[j][0] : prevPixel[j][0] - currPixel[j][0];
			diffPixel[j][1] = (currPixel[j][1] > prevPixel[j][1]) ? currPixel[j][1] - prevPixel[j][1] : prevPixel[j][1] - currPixel[j][1];
			diffPixel[j][2] = (currPixel[j][2] > prevPixel[j][2]) ? currPixel[j][2] - prevPixel[j][2] : prevPixel[j][2] - currPixel[j][2];

			if ((diffPixel[j][0] + diffPixel[j][1] + diffPixel[j][2]) > thresh) {
				diffPixel[j] = { 255,255,255 };
				diff.colDifftotals[j] += 1;
				diff.rowDifftotals[i] += 1;
			}
			else {
				diffPixel[j] = { 0,0,0 };
			}
		}
	}

	for (int i = 0; i < prev.rows; ++i) {
		if (diff.rowDifftotals[i] > diff.rowDiffmax) {
			diff.rowDiffmax = diff.rowDifftotals[i];
		}
	}
	for (int i = 0; i < prev.cols; ++i) {
		if (diff.colDifftotals[i] > diff.colDiffmax) {
			diff.colDiffmax = diff.colDifftotals[i];
		}
	}
}

std::vector<int> motionBox(diff& diff) {
	if (diff.rowDiffmax < 20 && diff.colDiffmax < 20) {
		return{};
	}
	int left, right, top, bottom;
	for (int i = 0; i < 480; ++i) {
		if (diff.rowDifftotals[i] >(double) (diff.rowDiffmax * (double)(box_thresh / box_thresh_max))) {
			top = i;
			break;
		}
	}
	for (int i = 479; i >= 0; --i) {
		if (diff.rowDifftotals[i] > (double)(diff.rowDiffmax * (double)(box_thresh / box_thresh_max))) {
			bottom = i;
			break;
		}
	}
	for (int i = 0; i < 640; ++i) {
		if (diff.colDifftotals[i] >(double) (diff.colDiffmax * (double)(box_thresh / box_thresh_max))) {
			left = i;
			break;
		}
	}
	for (int i = 639; i >= 0; --i) {
		if (diff.colDifftotals[i] > (double)(diff.colDiffmax * (double)(box_thresh / box_thresh_max))) {
			right = i;
			break;
		}
	}
	return{ left, right, top, bottom };
}


int main(int, char**)
{
	thresh = 36;
	box_thresh = 50;
	cv::VideoCapture cap(1); // open the default camera
	if (!cap.isOpened())  // check if we succeeded
		return -1;

	cv::Mat prev;
	bool first = true;

	cv::namedWindow("color", 1);
	cv::namedWindow("binary", 1);
	cv::namedWindow("ROI", 0);
	cv::resizeWindow("ROI", 640, 480);


	cv::createTrackbar("Threshold Value", "binary", &thresh, thresh_max);
	cv::createTrackbar("Linethreshold", "color", &box_thresh, box_thresh_max);
	cap >> prev;
	for (;;)
	{
		cv::Mat curr;
		
		diff diff;
		cap >> curr;
		diff.mat = cv::Mat(curr.size(), curr.type());
		cv::GaussianBlur(curr, curr, cv::Size(7, 7), 1.5, 1.5);

		calcDiff(prev, curr, diff);
		curr.copyTo(prev);

		std::vector<int> box;
		box = motionBox(diff);

		if (!box.empty()) {
			cv::Rect bound{ box[0],box[2], box[1] - box[0],box[3] - box[2] };
			if (bound.area() > 300) {
				cv::Mat roiColor = curr(bound);
				cv::Mat roiBinary = diff.mat(bound);
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
					reducedCountours.push_back(std::make_pair(temp , *i));
				}
				std::sort(reducedCountours.begin(), reducedCountours.end(), order);
				std::cout << reducedCountours.size()<< std::endl;
				for (int i = 0; i < reducedCountours.size(); ++i) {
					if (reducedCountours[i].first < max / 10) {
						reducedCountours.erase(reducedCountours.begin());
					}
				}
				std::cout << reducedCountours.size() << "\n\n\n" << std::endl;
				std::vector<std::vector<cv::Point>> new_contours;
				for (auto i = reducedCountours.begin(); i != reducedCountours.end(); ++i) {
					new_contours.push_back(i->second);
				}



				for (int i = 0; i < new_contours.size(); ++i) {
					cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
					cv::drawContours(roiBinary, new_contours, i, color, 2, 8, hierarchy, 0, cv::Point());
				}
				
				//clean up the block above this
				
				cv::Mat bufferH = cv::Mat(roiColor.rows, (curr.cols * 2 - roiColor.cols * 2), curr.type(), cv::Scalar(50, 50, 50));
				std::vector<cv::Mat> matrices = { roiColor,roiBinary,bufferH };
				cv::hconcat(matrices, bufferH);
				cv::Mat roiScaled = cv::Mat((curr.rows - bufferH.rows), bufferH.cols, curr.type(), cv::Scalar(50, 50, 50));
				cv::vconcat(bufferH, roiScaled, roiScaled);
				roiScaled.cols = curr.cols * 2;
				cv::imshow("ROI", roiScaled);
			}
			//std::cout << diff.colDiffmax << " " << diff.rowDiffmax << std::endl;
			cv::rectangle(curr, bound, CV_RGB(255, 0, 0), 2);
			cv::rectangle(diff.mat, bound, CV_RGB(255, 0, 0), 2);
		}

		cv::imshow("binary", diff.mat);
		cv::imshow("color", curr);
		if (cv::waitKey(5) >= 0) {}
	}
	// the camera will be deinitialized automatically in VideoCapture destructor
	return 0;
}

