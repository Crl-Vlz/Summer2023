#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

using namespace cv;
using namespace std;


int main()
{
	std::string image_path = samples::findFile("test2.jpeg");
	/////////
	Mat view = imread(image_path, IMREAD_GRAYSCALE);

	int height = view.rows, width = view.cols;
	int perMax = height * 2 + width * 2;
	int perMin = (perMax / 4);

	threshold(view, view, 150, 255, 0);

	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	vector<vector<Point>> contornosC;
	vector<Point> approxAux;
	vector<vector<Point>> approx;
	findContours(view, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);
	for (size_t i = 0; i < contours.size(); i++)
	{
		if (contours[(int)i].size() > perMin && contours[(int)i].size() < perMax)
		{
			contornosC.push_back(contours[(int)i]);
			drawContours(view, contours, (int)i, (Scalar)(255, 150, 150), 2, LINE_8, hierarchy, 0);
		}
	}

	for (int i = 0; i < contornosC.size(); i++)
	{
		approxPolyDP(contornosC[i], approxAux, contornosC[i].size() * 0.1, true); // epsilon 
		approx.push_back(approxAux);
	}

	Mat mask(view.rows, view.cols, CV_8U, Scalar(0));
	Mat view2 = view.clone();
	Mat multImage;
	Mat warp_dst = Mat::zeros(view.rows, view.cols, view.type());
	for (int i = 0; i < approx.size(); i++)
	{
		if (approx[i].size() == 4)
		{
			/////////////////////////////// Mascara
			for (int e = 0; e < mask.rows; e++)
			{
				for (int j = 0; j < mask.cols; j++)
				{
					if (pointPolygonTest(contornosC[i], Point(j, e), false) == true)
					{
						mask.at<uchar>(e, j) = (uchar)1;
					}
					else
					{
						mask.at<uchar>(e, j) = (uchar)0;
					}
				}
			}
			multImage = view2.mul(mask);
			multImage.copyTo(view);
			///////////////////////// Warp
			Point2f puntos[4];
			for (int a = 0; a < approx[i].size(); a++)
			{
				puntos[a] = approx[i][a];
			}

			// puntos destino
			Point2f dstTri[4];
			dstTri[0] = Point2f(puntos[0].x, puntos[0].y);
			dstTri[1] = Point2f(puntos[0].x, puntos[2].y);
			dstTri[2] = Point2f(puntos[2].x, puntos[2].y);
			dstTri[3] = Point2f(puntos[2].x, puntos[0].y);

			Mat warp_mat = getPerspectiveTransform(puntos, dstTri);

			warpPerspective(view, warp_dst, warp_mat, warp_dst.size());
		}
	}

	// Setup SimpleBlobDetector parameters.
	SimpleBlobDetector::Params params;
	// Change thresholds
	params.minThreshold = 50;
	params.maxThreshold = 150;

	params.blobColor = 1;

	// Filter by Area.
	params.filterByArea = true;
	params.minArea = 1200;  //circulos internos
	params.maxArea = 2800;
	//params.minArea =4000;  // circulos externos
	//params.maxArea = 7000;

	// circularity
	params.filterByCircularity = false;
	//params.minCircularity = .1;

	// Filter by Inertia
	params.filterByInertia = false;
	//params.minInertiaRatio = 0.01;
	std::vector<KeyPoint> keypoints;

	params.filterByColor = false;
	params.filterByConvexity = false;

	Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);

	Mat im_with_keypoints;
	detector->detect(warp_dst, keypoints);
	drawKeypoints(warp_dst, keypoints, im_with_keypoints, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	// Sort and draw lines
	sort(keypoints.begin(), keypoints.end(), [](KeyPoint a, KeyPoint b) { return a.pt.y < b.pt.y; });
	vector<KeyPoint> a;
	a.push_back(keypoints[0]);
	for (int i = 0; i < keypoints.size() - 1; i++) 
	{
		if (keypoints[i + 1].pt.y - keypoints[i].pt.y < 3) 
		{
			a.push_back(keypoints[i + 1]);
		}
		else 
		{
			sort(a.begin(), a.end(), [](KeyPoint a, KeyPoint b) { return a.pt.x < b.pt.x; });
			line(im_with_keypoints, Point(a[0].pt.x, a[0].pt.y), Point(a[a.size() - 1].pt.x, a[a.size() - 1].pt.y), (0, 0, 255), 4);
			a.clear();
			a.push_back(keypoints[i + 1]);
		}
		
	}
	a.push_back(keypoints[keypoints.size() - 1]);
	sort(a.begin(), a.end(), [](KeyPoint a, KeyPoint b) { return a.pt.x < b.pt.x; });
	line(im_with_keypoints, Point(a[0].pt.x, a[0].pt.y), Point(a[a.size() - 1].pt.x, a[a.size() - 1].pt.y), (0, 0, 255), 4);

	// Show blobs
	namedWindow("test", WINDOW_NORMAL);
	imshow("test", im_with_keypoints);


	waitKey(0);                                      	// Wait for a keystroke in the window
	return 0;
}
