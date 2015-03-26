#include "common.h"
#include "basic.h"
#include "function.h"

typedef unsigned char uchar;   ///实际在WinDef.h中 unsigned char也被typedef为BYTE
typedef unsigned int uint; 

//pts: i - (x,y)  
//labels: label((x,y)) = idx
//如果sfn!= "", 将i存储到mctable[idx]中, 并保存到sfn中
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
		cout << "Usage: MSLibrary.exe folder imfn1 imfn2 h" << endl;
		return 1;
	}

	string folder = argv[1];						// D:/RQRunning/Meanshift/Data/art/
	string imfn1 = folder + argv[2] + ".png";		// D:/RQRunning/Meanshift/Data/art/ + view1.png
	string imfn2 = folder + argv[3] + ".png";		// D:/RQRunning/Meanshift/Data/art/ + view5.png
	int h = string2type<int>(string(argv[4]));

	Mat im1 = imread(imfn1, 1);
	Mat im2 = imread(imfn2, 1);
	if (im1.data == NULL || im2.data == NULL)
	{
		cout << "failed to open " << imfn1 << " or " << imfn2 << endl;
		return -1;
	}
	
	Mat labim1, labim2;
	cvtColor(im1, labim1, CV_BGR2Lab);
	cvtColor(im2, labim2, CV_BGR2Lab);

	int width = im1.cols;
	int height = im1.rows;
	int ch = im1.channels();

	vector<uchar> imvec1(width*height*ch);
	vector<uchar> imvec2(width*height*ch);
	Mat2PixelsVector<uchar>(im1, imvec1);
	Mat2PixelsVector<uchar>(im2, imvec2);

	//1. 对target image进行分割
	vector<uchar> segim2(0); 
	vector<uchar> segbound2(0);
	vector<int> seglabels2(0);
	int regionum2;

	float sigmaS = 2*h+1;                          //spatial radius
	float sigmaR = 2*h;                            //color radius
	const int minR = 20;                           //min regions

	//对target image进行分割
	cout << "start mean-shift segmentation." << endl;
	cout << "spatial radius = " << sigmaS << endl;
	cout << "color radius = " << sigmaR << endl;

	Mat segmat2;

#define  RQ_DEBUG
#ifdef   RQ_DEBUG
	string segfn = folder + "seg_" + argv[3] + ".jpg";    //save segmentation image
	string lbfn = folder + "label_" + argv[3] + ".txt";   //save segmentation labels
	
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
	
	cout << "width = " << width << endl;
	cout << "height = " << height << endl;
	cout << "regionum = " << regionum2 << endl;

#else
	DoMeanShiftSegmentation(imvec2, width, height, ch, sigmaS, sigmaR, minR, segim2, segbound2, seglabels2, regionum2);
	DrawContoursAroundSegments(segim2, width, height, ch,  cv::Scalar(255,255,255));

	cout << "mean-shift segmentation for target image done." ;
	cout << argv[3] << " - " << regionum2 << " regions." << endl;

	//save segmentation results
	string segfn = folder + "seg_" + argv[3] + ".jpg";    //save segmentation image
	string lbfn = folder + "label_" + argv[3] + ".txt";   //save segmentation labels

	if ( segim2.size() == 0 )
	{
		cout << "wrong in segmentation." << endl;
		return -1;
	}

	//Mat segmat2;	
	PixelsVector2Mat(segim2, width, height, ch, segmat2);
	imwrite(segfn, segmat2);

	if (seglabels2.size() == 0)
	{
		cout << "wrong in get labels";
		return -1;

	}	
	saveLabels(seglabels2, width, height, lbfn);
#endif


	// 读入SIFT特征点
	// 读入SIFT特征点匹配，只保留有匹配的特征点
	// 统计每个区域的像素数目，每个区域的匹配数目
	//cout << "\n\tfeatures" << endl;
	
	//3.sift 
	/*vector <Point2f> sift1(0), sift2(0);
	string sftfn1 = folder + "SIFT/features_" + argv[2] + ".txt";
	string sftfn2 = folder + "SIFT/features_" + argv[3] + ".txt";
	string sfn1 = folder + "Sift_" + argv[2] + ".jpg";
	string sfn2 = folder + "Sift_" + argv[3] + ".jpg";
	string segsfn2 = folder + "Sift_seg_" + argv[3] + ".jpg";

	readSiftFeatures(sftfn1, sift1);
	readSiftFeatures(sftfn2, sift2);
	showFeatures(im1, sift1, cv::Scalar(122,25,25), sfn1 );
	showFeatures(im2, sift2, cv::Scalar(122,25,25), sfn2 );
	showFeatures(segmat2, sift2, cv::Scalar(122,25,25), segsfn2);
	cout << endl;*/
	
	//读入匹配
	cout << "\n\tmatches\n" << endl;

	vector<Point2f> sfmatchPt1(0), sfmatchPt2(0);
	string sfmfn = folder + "SIFT/matches.txt";
	string sfmsfn = folder + "Sift_matches.jpg";
	readSiftMatches(sfmfn, sfmatchPt1, sfmatchPt2);
	showMatches(im1, segmat2, sfmatchPt1, sfmatchPt2, sfmsfn);

	string sfn1 = folder + "valid_Sift_" + argv[2] + ".jpg";
	string sfn2 = folder + "valid_Sift_" + argv[3] + ".jpg";
	string segsfn2 = folder + "valid_Sift_seg_" + argv[3] + ".jpg";
	showFeatures(im1, sfmatchPt1, cv::Scalar(0,0,255), sfn1);
	showFeatures(im2, sfmatchPt2, cv::Scalar(0,0,255), sfn2);
	showFeatures(segmat2, sfmatchPt2, cv::Scalar(0,0,255), segsfn2 );
	cout << endl;
	
	//将匹配分配到对应的区域，存储匹配的下标
	//compute match table
	vector<vector<int>> sfmatchTable(regionum2);
	computeMatchTable(sfmatchPt2, seglabels2, width, sfmatchTable, "");

	string mskfolder = folder + "masks_" + argv[3] + "/";   //区域mask所在文件夹
	
	string smcntfn = folder + "regions_sift_matches.txt";     //保存每个区域的像素数目, 匹配点数目, 匹配点下标
	fstream fout(smcntfn , ios::out);
	if (fout.is_open() == NULL)
	{
		cout << "failed to open " << smcntfn << endl;
		return -1;
	}
	fout << regionum2 << endl;
		
	//统计无匹配区域数目，至少一个匹配区域数目，至少三个匹配区域数目
	int sfzerocnt = 0, sfonecnt = 0, sfthreecnt = 0;	
	for (int i = 0; i < regionum2; ++i)
	{
		string mskfn = mskfolder + "mask_" +  type2string<int>(i) + ".jpg";
		Mat msk = imread(mskfn);
		if (msk.data == NULL)
		{
			cout << "failed to open " << mskfn << endl;
			return -1;
		}
		Mat graymsk, binarymsk;
		cv::cvtColor(msk, graymsk, CV_BGR2GRAY);
		cv::threshold(graymsk, binarymsk, 125, 255, CV_THRESH_BINARY);

		/*保存分割后的每个区域
		string resfn = folder + "regions_" + argv[3] + "/region_" + type2string<int>(i) + "_" + argv[3] + ".jpg";
		Mat reim2 ;
		im2.copyTo(reim2, binarymsk);
		imwrite(resfn, reim2);
		continue;
		*/
		
		int pixelcnt = countNonZero(binarymsk);
		int matchcnt = sfmatchTable[i].size();
		
		fout << pixelcnt << ' ' << matchcnt << ' ';	
		
		if (matchcnt == 0)      sfzerocnt ++;
		if (matchcnt >= 1)		sfonecnt ++;
		if (matchcnt >= 3)  	sfthreecnt ++;

		for (int j = 0; j < matchcnt; ++j)
			fout << sfmatchTable[i][j] << ' ';
		fout << endl;	

		//显示保存每个区域内的匹配点
//#define SHOW_REGION
#ifdef SHOW_REGION
		Mat newim2(height, width, CV_8UC3) ;
		im2.copyTo(newim2, binarymsk);

		Mat canvas(height, width * 2, CV_8UC3);
		im1.copyTo(canvas(Rect(0,0,width,height)));
		newim2.copyTo(canvas(Rect(width,0,width,height)));

		string tempsfn = folder + "valid_region_sift_matches/regions_matches_" + type2string(i) + ".jpg";

		//加上匹配点
		for (int j = 0; j < matchcnt; ++j)
		{
			int ptidx = sfmatchTable[i][j];
			Point2i pt1 = Point2i(sfmatchPt1[ptidx].x , sfmatchPt1[ptidx].y);
			Point2i pt2 = Point2i(sfmatchPt2[ptidx].x + width, sfmatchPt2[ptidx].y);

			cv::line(canvas, pt1, pt2, cv::Scalar(0, 255, 0));
			cv::circle(canvas, pt1, 5, cv::Scalar(255, 0, 0));
			cv::circle(canvas, pt2, 5, cv::Scalar(255,0,0));
		}

		imwrite(tempsfn, canvas);
#endif		
	}

	cout << "sift matches: " << sfzerocnt << " regions have no matches." << endl;
	cout << "sift matches: " << sfonecnt  << " regions have less one matches." << endl;
	cout << "sift matches: " << sfthreecnt << " regions have less three matches." << endl;
	cout << "save matches in each regions done." << endl;

	//区域像素表: 每个区域内的像素 
	//compute pixeltable
	vector<vector<Point2f>> pixelTable2(regionum2);
	for(int i = 0; i < seglabels2.size(); ++i)
	{
		Point2f pt(i%width, i/width);	
		pixelTable2[seglabels2[i]].push_back(pt);
	}

	//color transfer region by region
	//所有大于3个匹配的区域进行对应区域的计算

	//测试用第68个区域-人像区域
	int idx = 730;
	vector<Point2f> cor_pixels1(0);   //correspond pixels in image1 
	Mat cor_mask1; 
	testTransform(idx, im1, im2, pixelTable2, sfmatchTable, sfmatchPt1, sfmatchPt2, cor_pixels1, cor_mask1 );

	//for (int i = 0; i < regionum2; ++i)
	//{
	//	if (sfmatchTable[i].size() >= 4)
	//	{
	//		int idx = i;
	//		vector<Point2f> cor_pixels1(0);   //correspond pixels in image1 
	//		Mat cor_mask1;                    //correspond mask in image1
	//		testTransform(idx, im1, im2, pixelTable2, sfmatchTable, sfmatchPt1, sfmatchPt2, cor_pixels1, cor_mask1);
	//	}
	//}
		

	cout << " done." << endl;
	
	return 1;
}