#include "function.h"

void maskFromPixels(vector<Point2f> validpixels, int h, int w, Mat& out_mask)
{
	out_mask = Mat::zeros(h, w, CV_8UC1);
	for (int i = 0; i < validpixels.size(); ++i)
	{
		int x = validpixels[i].x;
		int y = validpixels[i].y;

		if (x>=w || x<0 || y>=h || y<0)
		{
			continue;
		}

		out_mask.at<uchar>(y,x) = 255;
	}

	//test
	/*imshow("mask from pixels", out_mask);
	waitKey(0);
	destroyWindow("mask from pixels");*/
}

//输出结果： 对应区域的所有像素坐标，以及对应区域的Mask
//out_repixels1: correspond pixels in image1
//out_remask1:   correspond mask in image1
void testTransform(int idx, Mat im1, Mat im2, vector<vector<Point2f>> pixelTable2, vector<vector<int>> sfmatchTable, vector<Point2f> allFeatures1, vector<Point2f> allFeatures2,
				vector<Point2f>& out_repixels1, Mat& out_remask1)
{
	cout << "use " << idx << "-th region to test." << endl;

	int height = im1.rows;
	int width = im1.cols;

	//该区域内的像素转换为二维点向量
	int repixelcnt = pixelTable2[idx].size();
	vector<Point2f> pixelpts2vec(repixelcnt);
	copy(pixelTable2[idx].begin(), pixelTable2[idx].end(),  pixelpts2vec.begin());
	cout << endl << pixelpts2vec.size() << " pixels in " << idx << " th region." << endl;

	//该区域内的匹配转换为二维点向量，以及二维点Mat
	int matchcnt = sfmatchTable[idx].size(); 

	//region points vector
	vector<Point2f> repts1vec(0);                           
	vector<Point2f> repts2vec(0);
	for (int i = 0; i < matchcnt; ++i)
	{
		int tidx = sfmatchTable[idx][i];
		repts1vec.push_back(allFeatures1[tidx]);
		repts2vec.push_back(allFeatures2[tidx]);
	}
	cout << endl << matchcnt << " matches in " << idx << " th region." << endl;

	Mat remask2;          //region mask
	maskFromPixels(pixelTable2[idx], height, width, remask2);                 

	string savefn;    
	fstream sfout;

#define PERSPECTIVE_TRANSFORM
#ifdef  PERSPECTIVE_TRANSFORM

	cout << endl << "perspective transform: " << endl;
	Mat persmtx  = findHomography(repts2vec, repts1vec, RANSAC);   // repts2vec: srcpoints,  repts1vec: dstpoints
	cout << "perspective matrix: " << endl;
	cout << persmtx  << endl;

	savefn = "perspective_matrix_" + type2string<int>(idx) + ".txt";     // perspective transform matrix
	sfout.open(savefn, ios::out);
	sfout << persmtx << endl;
	sfout.close();

	cout << "test perspective matrix is right or not." << endl;
	vector<Point2f> pers_repts1vec(0);
	perspectiveTransform(repts2vec, pers_repts1vec, persmtx);

	savefn = "perspective_features1_" + type2string<int>(idx) + ".txt";    // features2 transformed by perspective matrix
	sfout.open(savefn, ios::out);
	sfout << pers_repts1vec.size() << endl;
	for (int i = 0; i < pers_repts1vec.size() ; ++i)
	{
		sfout << pers_repts1vec[i].x << ' ' << pers_repts1vec[i].y << endl;
	}
	sfout.close();
	cout << "save " << savefn << endl;

	savefn = "perspective_sift_matches_" + type2string<int>(idx) + ".jpg";
	showMatches(im1, im2, pers_repts1vec, repts2vec, savefn );
	cout << "save " << savefn << endl;

	//对整个区域的像素做变换
	cout << "apply perspective transformation matrix to pixels in " << idx << "th region." << endl;
	vector<Point2f> pers_pixelpts1vec(0);    
	cv::perspectiveTransform(pixelpts2vec, pers_pixelpts1vec, persmtx); 

	savefn = "perspective_pixelspts1_" + type2string<int>(idx) + ".txt";    // pixels2 transform by perspective matrix
	sfout.open(savefn, ios::out);
	sfout << pers_pixelpts1vec.size() << endl;
	for (int i = 0; i < pers_pixelpts1vec.size(); ++i)
	{
		sfout << pers_pixelpts1vec[i].x << ' ' << pers_pixelpts1vec[i].y << endl;
	}
	sfout.close();
	
	out_repixels1.resize(pers_pixelpts1vec.size());
	copy(pers_pixelpts1vec.begin(), pers_pixelpts1vec.end(), out_repixels1.begin());
	maskFromPixels(out_repixels1, height, width, out_remask1);

	savefn = "perspective_mask_" + type2string<int>(idx) + ".jpg";
	imwrite(savefn, out_remask1);
	savefn = "perspective_region_" + type2string<int>(idx) + ".jpg";
	Mat pers_reim1;
	im1.copyTo(pers_reim1, out_remask1);
	imwrite(savefn, pers_reim1);

	//local color transfer
	savefn = "perspective_lct_region_" + type2string<int>(idx) + ".jpg";
	RegionColorTransfer(im1, im2, out_remask1, remask2, savefn);
	cout << "perspective transform test done." << endl;

#endif


#define AFFINE_TRANSFORM
#ifdef  AFFINE_TRANSFORM

	cout << endl << "affine transform: " << endl;
	Mat affinemtx  =  estimateRigidTransform(repts2vec, repts1vec, true);  //求解的变换只限制于旋转，平移，缩放的组合
	cout << "affinemtx: " << affinemtx << endl;

	savefn = "affine_matrix_" + type2string<int>(idx) + ".txt";     // perspective transform matrix
	sfout.open(savefn, ios::out);
	sfout << affinemtx << endl;
	sfout.close();

	cout << "test affine matrix is right or not." << endl;
	vector<Point2f> affine_repts1vec(0);
	transform(repts2vec, affine_repts1vec, affinemtx);

	savefn = "affine_features1_" + type2string<int>(idx) + ".txt";    // features2 transformed by perspective matrix
	sfout.open(savefn, ios::out);
	sfout << affine_repts1vec.size() << endl;
	for (int i = 0; i < affine_repts1vec.size() ; ++i)
	{
		sfout << affine_repts1vec[i].x << ' ' << affine_repts1vec[i].y << endl;
	}
	sfout.close();
	cout << "save " << savefn << endl;

	savefn = "affine_sift_matches_" + type2string<int>(idx) + ".jpg";
	showMatches(im1, im2, affine_repts1vec, repts2vec, savefn );
	cout << "save " << savefn << endl;

	//对整个区域的像素做仿射变换
	cout << "apply affine transformation matrix to pixels in " << idx << "th region." << endl;
	vector<Point2f> aff_pixelpts1vec(0);    
	cv::transform(pixelpts2vec, aff_pixelpts1vec, affinemtx);   // pixelpts1vec = affinemtx * pixelpts2vec // pixelpts1vec = affinemtx * pixelpts2vec

	savefn = "affine_pixelspts1_" + type2string<int>(idx) + ".txt";    // pixels2 transform by perspective matrix
	sfout.open(savefn, ios::out);
	sfout << aff_pixelpts1vec.size() << endl;
	for (int i = 0; i < aff_pixelpts1vec.size(); ++i)
	{
		sfout << aff_pixelpts1vec[i].x << ' ' << aff_pixelpts1vec[i].y << endl;
	}
	sfout.close();

	//由aff_pixelspt1vec求解对应区域的mask
	out_repixels1.resize(aff_pixelpts1vec.size());
	copy(aff_pixelpts1vec.begin(), aff_pixelpts1vec.end(), out_repixels1.begin());
	maskFromPixels(out_repixels1, height, width, out_remask1);

	savefn = "affine_mask_" + type2string<int>(idx) + ".jpg";
	imwrite(savefn, out_remask1);
	savefn = "affine_region_" + type2string<int>(idx) + ".jpg";
	Mat aff_reim1;
	im1.copyTo(aff_reim1, out_remask1);
	imwrite(savefn, aff_reim1);

	//local color transfer
	savefn = "affine_lct_region_" + type2string<int>(idx) + ".jpg";
	RegionColorTransfer(im1, im2, out_remask1, remask2, savefn);
	cout << "affine transform test done." << endl;

#endif


}