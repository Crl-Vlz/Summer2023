#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
using namespace cv;
using namespace std;
void drawCircle(Mat img, Point center, int radius, int thickness) {
    cv::circle(img, center, radius, Scalar(0, 0, 0), thickness);
}
int main()
{
    int width = 1016, height = 1016;
    Mat image(height, width, CV_8UC3, Scalar(255, 255, 255));
    int dpi = 600;
    float circleRadius = 50;
    float spacing = 10;
    int maxW = (width / (circleRadius * 2))/2;
    int countX = 0;
    int outerCircle = circleRadius/2.5;
    int innerCircle = -1;
    for (int i = 1; i < (height / (circleRadius * 2))+1; i++) //filas
    {
        for (int e = 1; e < (width / (circleRadius * 2))+1; e++) //columnas
        {
            if (((e * circleRadius * 2) - circleRadius + spacing * e) < width && ((i * circleRadius * 2) - circleRadius + spacing * i) < height)
            {
                if ((i + e) % 2 == 1)
                {
                    drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e+20, (i * circleRadius * 2) - circleRadius + spacing * i+20), circleRadius,outerCircle);
                    countX++;
                    ////// dibujo el circulo 1
                    if (countX==1)
                    {
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius/5,innerCircle);
                    }
                    if (countX == maxW)
                    {
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e -40, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius / 5, innerCircle);
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e +40, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius / 5, innerCircle);
                    }
                    if (i == floor(width / (circleRadius * 2)) && countX == (maxW * (floor(width / (circleRadius * 2)) - 1)) + 1)
                    {
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e -40, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius/5 , innerCircle);
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e+40, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius/5, innerCircle);
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e, (i * circleRadius * 2) - circleRadius + spacing * i-40), circleRadius/5, innerCircle);
                    }
                    if (i == floor(width / (circleRadius * 2)) && countX == maxW * floor(width / (circleRadius * 2)))
                    {
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e-40, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius /5, innerCircle);
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e+40, (i * circleRadius * 2) - circleRadius + spacing * i), circleRadius /5, innerCircle);
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e, (i * circleRadius * 2) - circleRadius + spacing * i-40), circleRadius /5, innerCircle);
                        drawCircle(image, Point((e * circleRadius * 2) - circleRadius + spacing * e, (i * circleRadius * 2) - circleRadius + spacing * i+40), circleRadius /5, innerCircle);
                    }
                }
            }
        }
    }
    imwrite("test.jpg", image);
    imshow("Wah", image);
    waitKey(0);                                      	// Wait for a keystroke in the window
    return 0;
}
