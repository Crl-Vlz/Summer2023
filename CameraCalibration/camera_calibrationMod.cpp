#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdio>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#define _CRT_SECURE_NO_WARNINGS
using namespace cv;
using namespace std;
static void help()
{
    cout <<  "This is a camera calibration sample." << endl
         <<  "Usage: camera_calibration [configuration_file -- default ./default.xml]"  << endl
         <<  "Near the sample file you'll find the configuration file, which has detailed help of "
             "how to edit it.  It may be any OpenCV supported file format XML/YAML." << endl;
}
class Settings
{
public:
    Settings() : goodInput(false) {}
    enum Pattern { NOT_EXISTING, ASYMMETRIC_CIRCLES_GRID };
    enum InputType { INVALID, CAMERA, VIDEO_FILE, IMAGE_LIST };
    void write(FileStorage& fs) const                        //Write serialization for this class
    {
        fs << "{"
                  << "BoardSize_Width"  << boardSize.width
                  << "BoardSize_Height" << boardSize.height
                  << "Square_Size"         << squareSize
                  << "Calibrate_Pattern" << patternToUse
                  << "Calibrate_NrOfFrameToUse" << nrFrames
                  << "Calibrate_FixAspectRatio" << aspectRatio
                  << "Calibrate_AssumeZeroTangentialDistortion" << calibZeroTangentDist
                  << "Calibrate_FixPrincipalPointAtTheCenter" << calibFixPrincipalPoint
                  << "Write_DetectedFeaturePoints" << writePoints
                  << "Write_extrinsicParameters"   << writeExtrinsics
                  << "Write_outputFileName"  << outputFileName
                  << "Show_UndistortedImage" << showUndistorsed
                  << "Input_FlipAroundHorizontalAxis" << flipVertical
                  << "Input_Delay" << delay
                  << "Input" << input
           << "}";
    }
    void read(const FileNode& node)                          //Read serialization for this class
    {
        node["BoardSize_Width" ] >> boardSize.width;
        node["BoardSize_Height"] >> boardSize.height;
        node["Calibrate_Pattern"] >> patternToUse;
        node["Square_Size"]  >> squareSize;
        node["Calibrate_NrOfFrameToUse"] >> nrFrames;
        node["Calibrate_FixAspectRatio"] >> aspectRatio;
        node["Write_DetectedFeaturePoints"] >> writePoints;
        node["Write_extrinsicParameters"] >> writeExtrinsics;
        node["Write_outputFileName"] >> outputFileName;
        node["Calibrate_AssumeZeroTangentialDistortion"] >> calibZeroTangentDist;
        node["Calibrate_FixPrincipalPointAtTheCenter"] >> calibFixPrincipalPoint;
        node["Calibrate_UseFisheyeModel"] >> useFisheye;
        node["Input_FlipAroundHorizontalAxis"] >> flipVertical;
        node["Show_UndistortedImage"] >> showUndistorsed;
        node["Input"] >> input;
        node["Input_Delay"] >> delay;
        node["Fix_K1"] >> fixK1;
        node["Fix_K2"] >> fixK2;
        node["Fix_K3"] >> fixK3;
        node["Fix_K4"] >> fixK4;
        node["Fix_K5"] >> fixK5;
        validate();
    }
    void validate()
    {
        goodInput = true;
        if (boardSize.width <= 0 || boardSize.height <= 0)
        {
            cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
            goodInput = false;
        }
        if (squareSize <= 10e-6)
        {
            cerr << "Invalid square size " << squareSize << endl;
            goodInput = false;
        }
        if (nrFrames <= 0)
        {
            cerr << "Invalid number of frames " << nrFrames << endl;
            goodInput = false;
        }
        if (input.empty())      // Check for valid input
                inputType = INVALID;
        else
        {
            if (input[0] >= '0' && input[0] <= '9')
            {
                stringstream ss(input);
                ss >> cameraID;
                inputType = CAMERA;
            }
            else
            {
                if (isListOfImages(input) && readStringList(input, imageList))
                {
                    inputType = IMAGE_LIST;
                    nrFrames = (nrFrames < (int)imageList.size()) ? nrFrames : (int)imageList.size();
                }
                else inputType = VIDEO_FILE;
            }
            if (inputType == CAMERA) inputCapture.open(cameraID);
            if (inputType == VIDEO_FILE)inputCapture.open(input);
            if (inputType != IMAGE_LIST && !inputCapture.isOpened())inputType = INVALID;
        }
        if (inputType == INVALID)
        {
            cerr << " Input does not exist: " << input;
            goodInput = false;
        }
        flag = 0;
        if(calibFixPrincipalPoint) flag |= CALIB_FIX_PRINCIPAL_POINT;
        if(calibZeroTangentDist)   flag |= CALIB_ZERO_TANGENT_DIST;
        if(aspectRatio)            flag |= CALIB_FIX_ASPECT_RATIO;
        if(fixK1)                  flag |= CALIB_FIX_K1;
        if(fixK2)                  flag |= CALIB_FIX_K2;
        if(fixK3)                  flag |= CALIB_FIX_K3;
        if(fixK4)                  flag |= CALIB_FIX_K4;
        if(fixK5)                  flag |= CALIB_FIX_K5;
        calibrationPattern = NOT_EXISTING;
        if (!patternToUse.compare("ASYMMETRIC_CIRCLES_GRID")) calibrationPattern = ASYMMETRIC_CIRCLES_GRID;
        if (calibrationPattern == NOT_EXISTING)
        {
            cerr << " Camera calibration mode does not exist: " << patternToUse << endl;
            goodInput = false;
        }
        atImageList = 0;
    }
    Mat nextImage()
    {
        Mat result;
        if( inputCapture.isOpened() )
        {
            Mat view0;
            inputCapture >> view0;
            view0.copyTo(result);
        }
        else if( atImageList < imageList.size() ) result = imread(imageList[atImageList++], IMREAD_COLOR);
        return result;
    }
    static bool readStringList( const string& filename, vector<string>& l )
    {
        l.clear();
        FileStorage fs(filename, FileStorage::READ);
        if( !fs.isOpened() ) return false;
        FileNode n = fs.getFirstTopLevelNode();
        if( n.type() != FileNode::SEQ )return false;
        FileNodeIterator it = n.begin(), it_end = n.end();
        for( ; it != it_end; ++it )
            l.push_back((string)*it);
        return true;
    }
    static bool isListOfImages( const string& filename)
    {
        string s(filename);
        if( s.find(".xml") == string::npos && s.find(".yaml") == string::npos && s.find(".yml") == string::npos )
            return false;
        else
            return true;
    }
public:
    Size boardSize;              // The size of the board -> Number of items by width and height
    Pattern calibrationPattern;  // One of the Chessboard, circles, or asymmetric circle pattern
    float squareSize;            // The size of a square in your defined unit (point, millimeter,etc).
    int nrFrames;                // The number of frames to use from the input for calibration
    float aspectRatio;           // The aspect ratio
    int delay;                   // In case of a video input
    bool writePoints;            // Write detected feature points
    bool writeExtrinsics;        // Write extrinsic parameters
    bool calibZeroTangentDist;   // Assume zero tangential distortion
    bool calibFixPrincipalPoint; // Fix the principal point at the center
    bool flipVertical;           // Flip the captured images around the horizontal axis
    string outputFileName;       // The name of the file where to write
    bool showUndistorsed;        // Show undistorted images after calibration
    string input;                // The input ->
    bool useFisheye;             // use fisheye camera model for calibration
    bool fixK1;                  // fix K1 distortion coefficient
    bool fixK2;                  // fix K2 distortion coefficient
    bool fixK3;                  // fix K3 distortion coefficient
    bool fixK4;                  // fix K4 distortion coefficient
    bool fixK5;                  // fix K5 distortion coefficient
    int cameraID;
    vector<string> imageList;
    size_t atImageList;
    VideoCapture inputCapture;
    InputType inputType;
    bool goodInput;
    int flag;
private:
    string patternToUse;
};
static inline void read(const FileNode& node, Settings& x, const Settings& default_value = Settings())
{
    if(node.empty()) x = default_value;
    else x.read(node);
}
enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };
bool runCalibrationAndSave(Settings& s, Size imageSize, Mat&  cameraMatrix, Mat& distCoeffs,vector<vector<Point2f> > imagePoints );
int main(int argc, char* argv[])
{
    Settings s;
    s.boardSize = Size(4,11);              // The size of the board -> Number of items by width and height
    s.calibrationPattern = Settings::ASYMMETRIC_CIRCLES_GRID;  // One of the Chessboard, circles, or asymmetric circle pattern
    s.squareSize = 15;            // The size of a square in your defined unit (point, millimeter,etc).
    s.nrFrames = 10;                // The number of frames to use from the input for calibration
    s.aspectRatio = true;           // The aspect ratio
    s.delay = 0;                   // In case of a video input
    s.writePoints = false;            // Write detected feature points
    s.writeExtrinsics = true;        // Write extrinsic parameters
    s.calibZeroTangentDist = true;   // Assume zero tangential distortion
    s.calibFixPrincipalPoint = true; // Fix the principal point at the center
    s.flipVertical = false;           // Flip the captured images around the horizontal axis
    s.outputFileName = "calibracion.xml";       // The name of the file where to write
    s.showUndistorsed = true;        // Show undistorted images after calibration
    s.input = "0";                // The input ->
    s.useFisheye = false;             // use fisheye camera model for calibration
    s.fixK1 = false;                  // fix K1 distortion coefficient
    s.fixK2 = false;                  // fix K2 distortion coefficient
    s.fixK3 = false;                  // fix K3 distortion coefficient
    s.fixK4 = true;                  // fix K4 distortion coefficient
    s.fixK5 = true;                  // fix K5 distortion coefficient
    s.cameraID = 0;
    const string inputSettingsFile = argc > 1 ? argv[1] : "default.xml";
    FileStorage fs(inputSettingsFile, FileStorage::FORMAT_XML  ); // Read the settings
    if (!fs.isOpened())
    {
        cout << "Could not open the configuration file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }
    fs["Settings"] >> s;
    fs.release();                                         // close Settings file
   
    if (!s.goodInput)
    {
        cout << "Invalid input detected. Application stopping. " << endl;
        return -1;
    }
    vector<vector<Point2f> > imagePoints;
    Mat cameraMatrix, distCoeffs;
    Size imageSize;
    int mode = s.inputType == Settings::IMAGE_LIST ? CAPTURING : DETECTION;
    clock_t prevTimestamp = 0;
    const Scalar RED(0,0,255), GREEN(0,255,0);
    const char ESC_KEY = 27;
    int fotos = 0;
    Mat view[10];
    while(fotos<10)
    {
        Mat mostrar;
        s.inputCapture >> mostrar;
        imshow("Image View", mostrar);
        bool blinkOutput = false;
        char key = (char)waitKey(s.inputCapture.isOpened() ? 50 : s.delay);
        if (key == 'm')
        {
            view[fotos] = s.nextImage();
            fotos++;
        }
    }
    for (int i = 0; i < 10; i++)
    {
        vector<Point2f> pointBuf;
        if (findCirclesGrid(view[i], s.boardSize, pointBuf, CALIB_CB_ASYMMETRIC_GRID))
        {
            drawChessboardCorners(view[i], s.boardSize, Mat(pointBuf), true);
            imagePoints.push_back(pointBuf);
            imshow("Corners", view[i]);
            waitKey(500);
        }
    }
    imageSize = view[0].size();
    runCalibrationAndSave(s, imageSize, cameraMatrix, distCoeffs, imagePoints);
    for (int i = 0; i < 10; i++)
    {
        Mat temp = view[i].clone();
        undistort(temp, view[i], cameraMatrix, distCoeffs);
    }

    if( s.inputType == Settings::IMAGE_LIST && s.showUndistorsed )
    {
        Mat view, rview, map1, map2;
        initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0), imageSize,CV_16SC2, map1, map2);
        
        for(size_t i = 0; i < s.imageList.size(); i++ )
        {
            remap(view, rview, map1, map2, INTER_LINEAR);
            imshow("Image View", rview);
            char c = (char)waitKey();
            if( c  == ESC_KEY || c == 'q' || c == 'Q' )
                break;
        }
    }
    return 0;
}
static double computeReprojectionErrors( const vector<vector<Point3f> >& objectPoints,const vector<vector<Point2f> >& imagePoints,const vector<Mat>& rvecs, const vector<Mat>& tvecs, const Mat& cameraMatrix , const Mat& distCoeffs,vector<float>& perViewErrors, bool fisheye)
{
    vector<Point2f> imagePoints2;
    size_t totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());
    for(size_t i = 0; i < objectPoints.size(); ++i )
    {
        projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
        err = norm(imagePoints[i], imagePoints2, NORM_L2);
        size_t n = objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err*err/n);
        totalErr        += err*err;
        totalPoints     += n;
    }

    return std::sqrt(totalErr/totalPoints);
}
static void calcBoardCornerPositions(Size boardSize, float squareSize, vector<Point3f>& corners, Settings::Pattern patternType)
{
    corners.clear();
    switch(patternType)
    {
    case Settings::ASYMMETRIC_CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++ )
            for( int j = 0; j < boardSize.width; j++ )
                corners.push_back(Point3f((2*j + i % 2)*squareSize, i*squareSize, 0));
        break;
    default:
        break;
    }
}
static bool runCalibration( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,vector<vector<Point2f> > imagePoints, vector<Mat>& rvecs, vector<Mat>& tvecs,vector<float>& reprojErrs,  double& totalAvgErr)
{
    cameraMatrix = Mat::eye(3, 3, CV_64F);
    if( s.flag & CALIB_FIX_ASPECT_RATIO )
        cameraMatrix.at<double>(0,0) = s.aspectRatio;
    distCoeffs = Mat::zeros(8, 1, CV_64F);
    vector<vector<Point3f> > objectPoints(1);
    calcBoardCornerPositions(s.boardSize, s.squareSize, objectPoints[0], s.calibrationPattern);
    objectPoints.resize(imagePoints.size(),objectPoints[0]);
    double rms;
    rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs,s.flag);
    cout << "Re-projection error reported by calibrateCamera: "<< rms << endl;
    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);
    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix,distCoeffs, reprojErrs, s.useFisheye);
    return ok;
}
static void saveCameraParams( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,const vector<Mat>& rvecs, const vector<Mat>& tvecs,const vector<float>& reprojErrs, const vector<vector<Point2f> >& imagePoints,double totalAvgErr )
{
    FileStorage fs( s.outputFileName, FileStorage::WRITE );
    if( !rvecs.empty() || !reprojErrs.empty() ) fs << "nr_of_frames" << (int)std::max(rvecs.size(), reprojErrs.size());
    fs << "image_width" << imageSize.width;
    fs << "image_height" << imageSize.height;
    fs << "board_width" << s.boardSize.width;
    fs << "board_height" << s.boardSize.height;
    fs << "square_size" << s.squareSize;
    if( s.flag & CALIB_FIX_ASPECT_RATIO ) fs << "fix_aspect_ratio" << s.aspectRatio;
    if (s.flag)
    {
        std::stringstream flagsStringStream;
        flagsStringStream << "flags:" << (s.flag & CALIB_USE_INTRINSIC_GUESS ? " +use_intrinsic_guess" : "")<< (s.flag & CALIB_FIX_ASPECT_RATIO ? " +fix_aspectRatio" : "")<< (s.flag & CALIB_FIX_PRINCIPAL_POINT ? " +fix_principal_point" : "")<< (s.flag & CALIB_ZERO_TANGENT_DIST ? " +zero_tangent_dist" : "")<< (s.flag & CALIB_FIX_K1 ? " +fix_k1" : "")<< (s.flag & CALIB_FIX_K2 ? " +fix_k2" : "")<< (s.flag & CALIB_FIX_K3 ? " +fix_k3" : "")<< (s.flag & CALIB_FIX_K4 ? " +fix_k4" : "")<< (s.flag & CALIB_FIX_K5 ? " +fix_k5" : "");
        fs.writeComment(flagsStringStream.str());
    }
    fs << "flags" << s.flag;
    fs << "fisheye_model" << s.useFisheye;
    fs << "camera_matrix" << cameraMatrix;
    fs << "distortion_coefficients" << distCoeffs;
    fs << "avg_reprojection_error" << totalAvgErr;
    if (s.writeExtrinsics && !reprojErrs.empty()) fs << "per_view_reprojection_errors" << Mat(reprojErrs);
    if(s.writeExtrinsics && !rvecs.empty() && !tvecs.empty() )
    {
        CV_Assert(rvecs[0].type() == tvecs[0].type());
        Mat bigmat((int)rvecs.size(), 6, CV_MAKETYPE(rvecs[0].type(), 1));
        bool needReshapeR = rvecs[0].depth() != 1 ? true : false;
        bool needReshapeT = tvecs[0].depth() != 1 ? true : false;
        for( size_t i = 0; i < rvecs.size(); i++ )
        {
            Mat r = bigmat(Range(int(i), int(i+1)), Range(0,3));
            Mat t = bigmat(Range(int(i), int(i+1)), Range(3,6));
            if(needReshapeR) rvecs[i].reshape(1, 1).copyTo(r);
            else
            {
                CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
                r = rvecs[i].t();
            }
            if(needReshapeT) tvecs[i].reshape(1, 1).copyTo(t);
            else
            {
                CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
                t = tvecs[i].t();
            }
        }
        fs.writeComment("a set of 6-tuples (rotation vector + translation vector) for each view");
        fs << "extrinsic_parameters" << bigmat;
    }
    if(s.writePoints && !imagePoints.empty())
    {
        Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
        for( size_t i = 0; i < imagePoints.size(); i++ )
        {
            Mat r = imagePtMat.row(int(i)).reshape(2, imagePtMat.cols);
            Mat imgpti(imagePoints[i]);
            imgpti.copyTo(r);
        }
        fs << "image_points" << imagePtMat;
    }
}
bool runCalibrationAndSave(Settings& s, Size imageSize, Mat& cameraMatrix, Mat& distCoeffs,vector<vector<Point2f> > imagePoints)
{
    vector<Mat> rvecs, tvecs;
    vector<float> reprojErrs;
    double totalAvgErr = 0;
    bool ok = runCalibration(s, imageSize, cameraMatrix, distCoeffs, imagePoints, rvecs, tvecs, reprojErrs,totalAvgErr);
    cout << (ok ? "Calibration succeeded" : "Calibration failed") << ". avg re projection error = " << totalAvgErr << endl;
    if (ok)saveCameraParams(s, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, reprojErrs, imagePoints,totalAvgErr);
    return ok;
}