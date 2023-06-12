#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <iomanip>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <math.h>

using namespace cv;
using namespace std;


int main()
{
	/////////--------------- BLOB DETECTOR - 10 SMALL CIRCLES

	std::string image_path = samples::findFile("test.jpg");
	Mat view = imread(image_path, IMREAD_GRAYSCALE);

	// Setup SimpleBlobDetector parameters.
	SimpleBlobDetector::Params params;

	// Change thresholds
	params.minThreshold = 50;
	params.maxThreshold = 100;

	// Color of blobs
	params.blobColor = 0;

	// Filter by Area.
	params.filterByArea = true;
	params.minArea = 10;
	params.maxArea = 200;

	// Filter by Circularity
	//params.filterByCircularity = true;
	//params.minCircularity = 0.1;

	// Filter by Convexity
	//params.filterByConvexity = true;
	//params.minConvexity = 0.87;

	// Filter by Inertia
	//params.filterByInertia = true;
	//params.minInertiaRatio = 0.01;

	std::vector<KeyPoint> keypoints;
	Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);

	Mat im_with_keypoints;
	detector->detect(view, keypoints);
	drawKeypoints(view, keypoints, im_with_keypoints, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);


	/////////--------------- DISTANCES MATRIX

	float distancias[10][10];
	
	for(int i = 0; i < 10; i++) 
	{
		for (int j = 0; j < 10; j++) 
		{
			cout << fixed << setprecision(2);
			distancias[j][i] = sqrt(pow(keypoints[j].pt.x - keypoints[i].pt.x, 2) + pow(keypoints[j].pt.y - keypoints[i].pt.y, 2));
			cout << distancias[j][i] << "	";
		}
		cout << endl << endl;
	}

	
	

	/////////--------------- PRINT BLOB DETECTOR

	imshow("keypoints", im_with_keypoints);
	waitKey(0); // Wait for a keystroke in the window
	return 0;

}
