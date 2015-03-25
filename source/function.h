#include "common.h"
#include "basic.h"

void readLabels(string lbfn, int& w, int& h, int& regionum, vector<int>& labels);
void saveLabels(vector<int> labels, int w, int h, string lbfn);

void readFeatures(string ffn, vector<Point2f>& features);
void readSiftFeatures(string sffn, vector<Point2f>& features);
void showFeatures(Mat im, vector<Point2f>& features, cv::Scalar color, string sfn);

void readMatches(string mfn, vector<Point2f>& features1, vector<Point2f>& features2);
void readSiftMatches(string sfmfn, vector<Point2f>& features1, vector<Point2f>& features2 );
void showMatches(Mat im1, Mat im2, vector<Point2f>& pts1, vector<Point2f>& pts2, string smfn);

//mean-shift
void DrawContoursAroundSegments(vector<uchar>&	segmentedImage,	const int width, const int height, const int ch, const Scalar color);
void DoMeanShiftSegmentation(vector<uchar> inputimg, const int width, const int height, const int ch, const int sigmaS, const float sigmaR, const int minregion, 
	vector<uchar>& segimg, vector<uchar>& segbound, vector<int>& seglabels, int& regionum);


//perspective transform
void perspectivePoints(vector<Point2f> srcpts, Mat persmtx,  vector<Point2f>& dstpts);

//local color transfer
void RegionColorTransfer(Mat im1, Mat im2, Mat remsk1, Mat remsk2, string lctsfn);



