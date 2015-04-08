#include "common.h"
#include "basic.h"
#include "function.h"

typedef unsigned char uchar;   ///实际在WinDef.h中 unsigned char也被typedef为BYTE
typedef unsigned int uint; 

void ComputeRegionMatches(vector<int> labels2, int regionum2, int step, vector<Point2f> features1, vector<Point2f> features2, vector<vector<Point2f>>& matches1, vector<vector<Point2f>>& matches2)
{
	matches1.resize(regionum2);
	matches2.resize(regionum2);

	for (int i = 0; i < features2.size(); ++i)
	{
		Point2f pt2 = features2[i];
		Point2f pt1 = features1[i];

		int idx  = labels2[pt2.y * step + pt2.x];
		matches1[idx].push_back(pt1);
		matches2[idx].push_back(pt2);
	}
}

void computeMatchTable( vector<Point2f> pts, vector<int> labels, int step, vector<vector<int>>& mctable, string sfn)
{
	for (int i = 0; i < pts.size(); ++i)
	{
		Point2i pt = pts[i];		
		int idx = labels[pt.y * step + pt.x];
		mctable[idx].push_back(i);
	}

	if (sfn != "")
	{
		fstream fout(sfn, ios::out);
		if (fout.is_open() == false)
		{
			cout << "failed to open " << sfn << endl;
			return;
		}

		//int zerocnt = 0;

		fout << mctable.size() << endl;
		for (int i = 0; i < mctable.size(); ++i)
		{
			/*	if (mctable[i].size() == 0)
			{
				zerocnt ++;
			}
			*/
			fout << mctable[i].size() << ' ';
			for (int j = 0; j < mctable[i].size(); ++j)
				fout << mctable[i][j] << ' ';
			fout << endl;
		}
		fout.close();
	}

}

int main(int argc, char *argv[])
{
	if (argc != 5)
	{
		/*运行程序，图像数据目录，原图像，目标图像，ms算法分割参数*/
		cout << "Usage: MSLibrary.exe folder imfn1 imfn2 outfolder" << endl;
		return 1;
	}

	string folder = argv[1];						// D:/RQRunning/Meanshift/Data/art/
	string imfn1 = folder + argv[2] + ".png";		// D:/RQRunning/Meanshift/Data/art/ + view1.png
	string imfn2 = folder + argv[3] + ".png";		// D:/RQRunning/Meanshift/Data/art/ + view5.png
	
	string outfolder = "../output";
	_mkdir(outfolder.c_str());
	outfolder = outfolder + "/" + argv[4];
	_mkdir(outfolder.c_str());


	//算法部分

	Mat im1 = imread(imfn1, 1);
	Mat im2 = imread(imfn2, 1);
	if (im1.data == NULL || im2.data == NULL)
	{
		cout << "failed to open " << imfn1 << " or " << imfn2 << endl;
		return -1;
	}
	
	//---------------------------------------------------------------------------------------------------------//
	//---------------------------------------Global Color Transfer---------------------------------------------//
		Mat gct_newim2;
		GlobalColorTransfer(im1, im2, gct_newim2, outfolder + "/gct_view5E.png");
	//---------------------------------------------------------------------------------------------------------//

	//---------------------------------------------------------------------------------------------------------//
	//---------------------------------------ACCV2009----------------------------------------------------------//
	//见accv2009
	//cout << "result of ACCV2009" << endl;
	//FeaturesColorCorrection(im1, im2, pixelTable2, sfmatchTable, sfmatchPt1, sfmatchPt2, folder, argv[3]);
	//---------------------------------------------------------------------------------------------------------//
	
	int width = im1.cols;
	int height = im1.rows;
	int ch = im1.channels();

	vector<uchar> imvec1(width*height*ch);
	vector<uchar> imvec2(width*height*ch);
	Mat2PixelsVector<uchar>(im1, imvec1);
	Mat2PixelsVector<uchar>(im2, imvec2);


	//分割: meanshift
	vector<uchar> segim2(0); 
	vector<uchar> segbound2(0);
	vector<int> seglabels2(0);
	vector<vector<Point2f>> pixelTable2(0);

	Mat segmat2;
	int regionum2;

	//float sigmaS = 2*h;
	//float sigmaR = 2*h+1;
	//const int minR = 20;

	float sigmaS = 5;                   //spatial radius
	float sigmaR = 10;                  //color radius
	const int minR = 800;               //min regions

//#define  RQ_DEBUG
#ifdef   RQ_DEBUG
	//直接读入分割结果
	string segfn = outfolder + "/seg_" + argv[3] + ".jpg";    //save segmentation image
	string lbfn  = outfolder + "/label_" + argv[3] + ".txt";   //save segmentation labels
	
	segmat2 = imread(segfn);
	if (segmat2.data == NULL)
	{
		cout << "failed to open " << segfn << endl;
		return -1 ;
	}
	readLabels(lbfn, width, height, regionum2, seglabels2);

	cout << endl;
	cout << "read target image labels done."  << endl;
	cout << "load target image segmentation result done." << endl;
	
	cout << "width = " << width <<  endl;
	cout << "height = " << height << endl;
	cout << "regionum = " << regionum2 << endl;

	//计算区域像素表 
	pixelTable2.resize(regionum2);
	for(int i = 0; i < seglabels2.size(); ++i)
	{
		Point2f pt(i%width, i/width);	
		pixelTable2[seglabels2[i]].push_back(pt);
	}
#else
	//进行mean-shift分割
	cout << "start mean-shift segmentation." << endl;
	cout << "spatial radius = " << sigmaS << endl;
	cout << "color radius = " << sigmaR << endl;
	cout << "min region = " << minR << endl;
	
	DoMeanShiftSegmentation(im2, sigmaS, sigmaR, minR, segim2, seglabels2, regionum2);
	DrawContoursAroundSegments(segim2, width, height, ch,  cv::Scalar(255,255,255));

	cout << "mean-shift segmentation for target image done." ;
	cout << argv[3] << " - " << regionum2 << " regions." << endl;

	//save segmentation results
	string segfn = outfolder + "/seg_" + argv[3] + ".jpg";    //save segmentation image
	string lbfn  = outfolder + "/label_" + argv[3] + ".txt";   //save segmentation labels

	if ( segim2.size() == 0 )
	{
		cout << "wrong in segmentation." << endl;
		return -1;
	}

	//保存分割的标签分配结果
	saveLabels(seglabels2, width, height, lbfn);

	//保存分割的图像结果
	PixelsVector2Mat(segim2, width, height, ch, segmat2);
	imwrite(segfn, segmat2);
	
	//计算每个区域的像素表 
	pixelTable2.resize(regionum2);
	for(int i = 0; i < seglabels2.size(); ++i)
	{
		Point2f pt(i%width, i/width);	
		pixelTable2[seglabels2[i]].push_back(pt);
	}

	//保存每个区域图像和对应mask
	string mskfolder = outfolder + "/masks_" + argv[3];
	string regionfolder = outfolder + "/regions_" + argv[3];
	_mkdir(mskfolder.c_str());
	_mkdir(regionfolder.c_str());

	for (int i = 0; i < pixelTable2.size(); ++i)
	{
		string mskfn =  mskfolder + "/mask_" +  type2string<int>(i) + ".jpg";
		Mat msk;
		maskFromPixels(pixelTable2[i], height, width, msk);
		imwrite(mskfn, msk);

		string resfn = regionfolder + "/region_" + type2string<int>(i) + "_" + argv[3] + ".jpg";
		Mat reim2 ;
		im2.copyTo(reim2, msk);
		imwrite(resfn, reim2);		
	}
#endif
		
	//区域内特征匹配 -> 区域内像素对应
	cout << "\n\tmatches\n" << endl;

	vector<Point2f> matchPts1(0), matchPts2(0);
	readAllMatches(folder, matchPts1, matchPts2);
	
	string sfn1 = outfolder + "/features_" + argv[2] + ".jpg";
	string sfn2 = outfolder + "/features_" + argv[3] + ".jpg";
	string segsfn2 = outfolder  + "/features_seg_" + argv[3] + ".jpg";
	showFeatures(im1, matchPts1, cv::Scalar(0,0,255), sfn1);
	showFeatures(im2, matchPts2, cv::Scalar(0,0,255), sfn2);
	showFeatures(segmat2, matchPts1, cv::Scalar(0,0,255), segsfn2 );
	cout << endl;
	
	//每个区域的有那些匹配
	vector<vector<Point2f>> matchTable1(0), matchTable2(0);
	ComputeRegionMatches(seglabels2, regionum2, width, matchPts1, matchPts2, matchTable1, matchTable2 );

	string mtfn = outfolder + "/regions_matches.txt";
	fstream fout(mtfn.c_str(), ios::out);
	if (fout.is_open() == false)
	{
		cout << "failed to open " << mtfn << endl;
		return -1;
	}
	fout << regionum2 << endl;
	
	int zerocnt = 0;     //区域内无匹配
	int onecnt  = 0;     //区域内有至少一个匹配
	int threecnt = 0;    //区域内至少三个匹配
	for (int i = 0; i < regionum2; ++i )
	{
		Mat msk;
		maskFromPixels(pixelTable2[i], height, width, msk);

		int pixelcnt = countNonZero(msk);
		int matchcnt = matchTable2[i].size();

		fout << i << "th region: "<< pixelcnt << "pixels  " << matchcnt << " matches." << endl  ;
		if (matchcnt == 0)     zerocnt ++;
		if (matchcnt >= 1)	   onecnt ++;
		if (matchcnt >= 3)     threecnt ++;

#define SHOW_REGION
#ifdef  SHOW_REGION

		Mat tmp(height, width, CV_8UC3);
		im2.copyTo(tmp, msk);

		Mat canvas(height, width * 2, CV_8UC3);
		im1.copyTo(canvas(Rect(0,0,width,height)));
		tmp.copyTo(canvas(Rect(width,0,width,height)));

		string tmpfolder = outfolder + "/regions_matches";
		_mkdir(tmpfolder.c_str());

		string tempsfn = tmpfolder + "/matches_" + type2string(i) + ".jpg";

		//显示匹配点
		for (int j = 0; j < matchcnt; ++j)
		{			
			Point2i pt1 = Point2i(matchTable1[i][j].x , matchTable1[i][j].y);
			Point2i pt2 = Point2i(matchTable2[i][j].x + width, matchTable2[i][j].y);

			cv::line(canvas, pt1, pt2, cv::Scalar(0, 255, 0));
			cv::circle(canvas, pt1, 5, cv::Scalar(255, 0, 0));
			cv::circle(canvas, pt2, 5, cv::Scalar(255, 0, 0));
		}
		imwrite(tempsfn, canvas);
#endif		
	}
	cout << "save matches in each regions done." << endl;
	cout << "sift matches: " << zerocnt << " regions have no matches." << endl;
	cout << "sift matches: " << onecnt  << " regions have less one matches." << endl;
	cout << "sift matches: " << threecnt << " regions have less three matches." << endl;

	//---------------------------------------------------------------------------------------------------------//
	//--------------测试区域的透视变换以及颜色转移：用第68个区域-人像区域---------------------//
	//包含每个区域的perspective mask, perspective region
	//int idx = 68;
	//vector<Point2f> cor_pixels1(0);   //correspond pixels in image1 
	//Mat cor_mask1; 
	//testTransform(idx, im1, im2, pixelTable2, sfmatchTable, sfmatchPt1, sfmatchPt2, cor_pixels1, cor_mask1 );
	//return 11;
	//---------------------------------------------------------------------------------------------------------//


	cout << endl << "/******************find regions correspondence******************/" << endl;
	string tmpfolder = outfolder + "/regions_map";
	_mkdir(tmpfolder.c_str());

	vector<vector<Point2f>> pixelTable1(0);
	FindRegionMapping(im1, im2, matchTable2, matchTable1, pixelTable2, pixelTable1, tmpfolder);

	cout << endl << "/******************region color correction*********************/" << endl;
	LocalColorTransfer(im1, im2, pixelTable1, pixelTable2, outfolder, argv[3]);

	cout << endl << "/************region color correction: use of global************/" << endl;
	LocalColorTransfer2(im1, im2, pixelTable1, pixelTable2, outfolder, argv[3]);

	//衡量两幅图的相似性




	cout << "done" << endl;
	return 1;
}