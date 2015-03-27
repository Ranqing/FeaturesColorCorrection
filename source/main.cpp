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

	//compute pixel table:  每个区域内有那些像素（二维点）
	vector<vector<Point2f>> pixelTable2(regionum2);
	for(int i = 0; i < seglabels2.size(); ++i)
	{
		Point2f pt(i%width, i/width);	
		pixelTable2[seglabels2[i]].push_back(pt);
	}

	//测试用第68个区域-人像区域
	//包含每个区域的perspective mask, perspective region
	//int idx = 730;
	//vector<Point2f> cor_pixels1(0);   //correspond pixels in image1 
	//Mat cor_mask1; 
	//testTransform(idx, im1, im2, pixelTable2, sfmatchTable, sfmatchPt1, sfmatchPt2, cor_pixels1, cor_mask1 );
	
	Mat new_im2(height, width, CV_8UC3, cv::Scalar(0,0,0));

	cout << endl << "/******************find regions correspondence******************/" << endl;
	vector<vector<Point2f>> pixelTable1(0);   // 与pixelpixelTable2对应。。没有对应性的区域为空
	vector<int> matchedRegionsIdx(0);         // 求解得到匹配的区域index
	for (int i = 0; i < regionum2; ++i)
	{
		if (sfmatchTable[i].size() < 4)
		{
			pixelTable1.push_back(vector<Point2f>());
			continue;                   //can't compute a transform
		}

		int idx = i;
		int matchcnt = sfmatchTable[idx].size();

		vector<Point2f> repts1(0), repts2(0);  
		for (int j = 0; j < matchcnt; ++j)
		{
			int tidx = sfmatchTable[idx][j];
			repts1.push_back(sfmatchPt1[tidx]);
			repts2.push_back(sfmatchPt2[tidx]);
		}
		
		cout << endl << idx << "th-region: perspective transform" << endl;
		Mat persmtx  = findHomography(repts2, repts1, RANSAC);   
		cout << "perspective matrix: " << endl;
		cout << persmtx  << endl;

		cout << "apply perspective transformation matrix to pixels in " << idx << "th region." << endl; 
		vector<Point2f> pers_pixelpts1(0);
		cv::perspectiveTransform(pixelTable2[idx], pers_pixelpts1, persmtx);
		pixelTable1.push_back(pers_pixelpts1);

		Mat remsk1, remsk2;
		maskFromPixels(pixelTable1[idx], height, width, remsk1);
		maskFromPixels(pixelTable2[idx], height, width, remsk2);

		Mat reim1, reim2;
		im1.copyTo(reim1, remsk1);
		im2.copyTo(reim2, remsk2);
		Mat canvas(height, 2*width, CV_8UC3);
		reim1.copyTo(canvas(Rect(0,0,width,height)));
		reim2.copyTo(canvas(Rect(width,0,width,height)));

		string savefn = "correspond_region_" + type2string<int>(idx) + ".jpg";
		imwrite(savefn, canvas);

		matchedRegionsIdx.push_back(idx);
	}
	cout << endl << "/*******************find regions correspondence done******************/" << endl;
	
	//保存对应性结果
	//savePixelTable(pixelTable1, "pixelTable1.txt");
	//savePixelTable(pixelTable2, "pixelTable2.txt");

	//将pixelTable1的结果转换成为labels
	vector<int> seglabels1(height * width, -1);    
	for (int i = 0; i < pixelTable1.size(); ++i)
	{
		for (int j = 0; j < pixelTable1[i].size(); ++j)
		{
			Point2f tpt = pixelTable1[i][j];
			int y = min(max((int)tpt.y, 0), height-1);
			int x = min(max((int)tpt.x, 0), width -1);
			int idx = y * width + x;
			seglabels1[idx] = i;
		}
	}

	//保存标签结果
	//saveLabels(seglabels1, width, height, "labels1.txt");
	//cout << "pixelTable convert to labels done." << endl;
	//cout << "save labels1.txt" << endl;
	//return 1;

	//weighted local color transfer
	cout << endl << "/*******************weighted local color transfer******************/" << endl;
	
	Mat labim1, labim2;
	cvtColor(im1, labim1, CV_BGR2Lab);
	cvtColor(im2, labim2, CV_BGR2Lab);

	/*Scalar global_means1, global_stddvs1, global_means2, global_stddvs2;
	meanStdDev(labim1, global_means1, global_stddvs1);
	meanStdDev(labim2, global_means2, global_stddvs2);*/
	

	//所有有匹配的区域
	int matchedcnt = matchedRegionsIdx.size();	
	cout << matchedcnt  << " matched region." << endl;

	vector<float> Lfactors(matchedcnt), Afactors(matchedcnt), Bfactors(matchedcnt);	
	
	//每个区域的均值和方差
	vector<Scalar> means1(0), stddvs1(0);
	vector<Scalar> means2(0), stddvs2(0);

	for (int i = 0; i < matchedcnt; ++i)
	{
		int idx = matchedRegionsIdx[i];       //区域的index

		Mat remsk1, remsk2;
		maskFromPixels(pixelTable1[idx], height, width, remsk1);
		maskFromPixels(pixelTable2[idx], height, width, remsk2);

		Scalar tmpmean1, tmpstddv1;
		Scalar tmpmean2, tmpstddv2;
		
		meanStdDev(labim1, tmpmean1, tmpstddv1, remsk1);
		means1.push_back(tmpmean1);
		stddvs1.push_back(tmpstddv1);

		meanStdDev(labim2, tmpmean2, tmpstddv2, remsk2);
		means2.push_back(tmpmean2);
		stddvs2.push_back(tmpstddv2);

		Lfactors[i] = (stddvs1[i].val[0]) / (stddvs2[i].val[0]);
		Afactors[i] = (stddvs1[i].val[1]) / (stddvs2[i].val[1]);
		Bfactors[i] = (stddvs1[i].val[2]) / (stddvs2[i].val[2]);	
	}

	//颜色权重的参数
	double alpha  = 20;
	double alpha2 = alpha * alpha;

	Mat new_labim2(height, width, CV_8UC3, Scalar(0,0,0));

//#define WEIGHTED_LCT
#ifdef WEIGHTED_LCT
	for (int y = 0; y < height; ++ y)
	{
		for (int x = 0; x < width; ++ x)
		{
			Vec3b color = labim2.at<Vec3b>(y,x);       //原本的颜色

			double weightLsum = 0.0, weightAsum = 0.0, weightBsum = 0.0;
			Scalar colorsum(0.0,0.0,0.0);

			//所有有匹配的区域进行加权平均
			for (int i = 0; i < matchedcnt; ++i)
			{	
				//e(-(x*x)/2*(alpha*alpha))
				double weightL = exp(-0.5 *  (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]) / alpha2 );
				double weightA = exp(-0.5 *  (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]) / alpha2);
				double weightB = exp(-0.5 *  (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]) / alpha2);

				double newcolorL = weightL *( Lfactors[i] * (color[0] - means2[i].val[0]) + means1[i].val[0] );
				double newcolorA = weightA *( Afactors[i] * (color[1] - means2[i].val[1]) + means1[i].val[1] );
				double newcolorB = weightB *( Bfactors[i] * (color[2] - means2[i].val[2]) + means1[i].val[2] );

				weightLsum += weightL;
				weightAsum += weightA;
				weightBsum += weightB;

				colorsum.val[0] += newcolorL;
				colorsum.val[1] += newcolorA;
				colorsum.val[2] += newcolorB;
			}

			//类型转换
			if (weightLsum != 0)
				new_labim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,colorsum.val[0]/weightLsum), 255.0);
			if (weightAsum != 0)
				new_labim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,colorsum.val[1]/weightAsum), 255.0);
			if (weightBsum != 0)
				new_labim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,colorsum.val[2]/weightBsum), 255.0);
		}
	}
#else

	for (int i = 0; i < matchedcnt; ++i)
	{
		int idx = matchedRegionsIdx[i];

		double lfactor = Lfactors[i];
		double afactor = Afactors[i];
		double bfactor = Bfactors[i];

		//求第idx个区域的新颜色

		for (int j = 0; j < pixelTable2[idx].size(); ++j)
		{
			Point2f tpt = pixelTable2[idx][j];
			int y = tpt.y;
			int x = tpt.x;

			Vec3b color = labim2.at<Vec3b>(y,x);

			double newcolorL = lfactor * (color[0] - means2[i].val[0]) + means1[i].val[0];
			double newcolorA = afactor * (color[1] - means2[i].val[1]) + means1[i].val[1];
			double newcolorB = bfactor * (color[2] - means2[i].val[2]) + means1[i].val[2];

			new_labim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,newcolorL), 255.0);
			new_labim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,newcolorA), 255.0);
			new_labim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,newcolorB), 255.0);
		}

	}
#endif
	

	imshow("lab", new_labim2);
	waitKey(0);
	destroyWindow("lab");

	cout << endl << "/*******************weighted local color transfer done******************/" << endl;
	
	string savefn = folder + "weighted_lct_" + argv[3] + ".jpg";
	cvtColor(new_labim2, new_im2, CV_Lab2LBGR);
	imwrite(savefn, new_im2);
	cout << "save file " << savefn  << endl;

	return 1;
}