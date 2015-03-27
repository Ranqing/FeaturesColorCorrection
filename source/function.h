#include "common.h"
#include "basic.h"

void readLabels(string lbfn, int& w, int& h, int& regionum, vector<int>& labels);
void saveLabels(vector<int> labels, int w, int h, string lbfn);

//for test
void savePixelTable(vector<vector<Point2f>> pixeltable, string fn);
void readPixelTable(string fn, vector<vector<Point2f>>& pixeltable);


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

//generate a mask from valid pixels
void maskFromPixels(vector<Point2f> validpixels, int h, int w, Mat& out_mask);

// test the idea of Transform is work or not
void testTransform(int idx, Mat im1, Mat im2, vector<vector<Point2f>> pixelTable2, vector<vector<int>> sfmatchTable, vector<Point2f> allFeatures1, vector<Point2f> allFeatures2,
	vector<Point2f>& out_repixels1, Mat& out_remask1);


//local color transfer
void RegionColorTransfer(Mat im1, Mat im2, Mat remsk1, Mat remsk2, string lctsfn);





