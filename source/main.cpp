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

		/*string resfn = folder + "regions_" + argv[3] + "/region_" + type2string<int>(i) + "_" + argv[3] + ".jpg";
		Mat reim2 ;
		im2.copyTo(reim2, binarymsk);
		imwrite(resfn, reim2);
		continue;*/
		
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

	/*cout << "save regions done." << endl;
	return 1;*/
	
	cout << "sift matches: " << sfzerocnt << " regions have no matches." << endl;
	cout << "sift matches: " << sfonecnt  << " regions have less one matches." << endl;
	cout << "sift matches: " << sfthreecnt << " regions have less three matches." << endl;

	//target image的像素类别表， 每个region所包含的像素坐标 
	vector<vector<Point2f>> pixelTable2(regionum2);
	for(int i = 0; i < seglabels2.size(); ++i)
	{
		Point2f pt(i%width, i/width);	
		pixelTable2[seglabels2[i]].push_back(pt);
	}

	//测试用区域： 第68个区域-人像区域
	int idx = 68;  
	cout << "use " << idx << "-th region to test." << endl;	

	//读入该区域的mask
	string remskfn2 = mskfolder + "mask_" +  type2string<int>(idx) + ".jpg";
	Mat remsk2 = imread(remskfn2);
	if (remsk2.data == NULL)
	{
		cout << "failed to open " << remskfn2 << endl;
		return -1;
	}
	Mat gray_remsk2, binary_remsk2;
	cv::cvtColor(remsk2, gray_remsk2, CV_BGR2GRAY);
	cv::threshold(gray_remsk2, binary_remsk2, 125, 255, CV_THRESH_BINARY);
	cout << "read " << idx << " region's mask done." << endl;

	//该区域内的像素转换为二维点向量， 以及二维点Mat
	int repixelcnt = pixelTable2[idx].size();
	Mat pixelpts2(repixelcnt, 2, CV_32FC1);
	for (int i = 0; i < repixelcnt; ++i)
	{
		pixelpts2.at<float>(i,0) = (int)pixelTable2[idx][i].x;
		pixelpts2.at<float>(i,1) = (int)pixelTable2[idx][i].y;
	}
	vector<Point2f> pixelpts2vec(repixelcnt);
	copy(pixelTable2[idx].begin(), pixelTable2[idx].end(),  pixelpts2vec.begin());
	cout << endl << pixelpts2vec.size() << " pixels in " << idx << " th region." << endl;

	//该区域内的匹配转换为二维点向量，以及二维点Mat
	int matchcnt = sfmatchTable[idx].size(); 

	Mat repts1(matchcnt, 2, CV_32FC1);
	Mat repts2(matchcnt, 2, CV_32FC1);
	for (int i = 0; i < matchcnt; ++i)
	{
		int tidx = sfmatchTable[idx][i];
		repts1.at<float>(i, 0) = sfmatchPt1[tidx].x;
		repts1.at<float>(i, 1) = sfmatchPt1[tidx].y;

		repts2.at<float>(i, 0) = sfmatchPt2[tidx].x;
		repts2.at<float>(i, 1) = sfmatchPt2[tidx].y;
	}

	vector<Point2f> repts1vec(0);
	vector<Point2f> repts2vec(0);
	for (int i = 0; i < matchcnt; ++i)
	{
		int tidx = sfmatchTable[idx][i];
		repts1vec.push_back(sfmatchPt1[tidx]);
		repts2vec.push_back(sfmatchPt2[tidx]);
	}
	cout << endl << matchcnt << " matches in " << idx << " th region." << endl;
	
	/************************求透视变换********************************/

	cout << endl << "perspective transform: " << endl;

	Mat persmtx  = findHomography(repts2vec, repts1vec, RANSAC);   // repts2vec: srcpoints,  repts1vec: dstpoints
	cout << "perspective matrix: " << endl;
	cout << persmtx  << endl;

#define RQ_TESTIFY
#ifdef RQ_TESTIFY
	cout << "test perspective matrix is right or not." << endl;
	vector<Point2f> pers_repts1vec(0);
	perspectiveTransform(repts2vec, pers_repts1vec, persmtx);
	
	string perssfn = "repts1_transformby_repts2.txt";
	fstream persfout (perssfn, ios::out);
	persfout << pers_repts1vec.size() << endl;
	for (int i = 0; i < pers_repts1vec.size() ; ++i)
	{
		persfout << pers_repts1vec[i].x << ' ' << pers_repts1vec[i].y << endl;
	}
	persfout.close();

	perssfn = "test_pers_sift_matches_68.jpg";
	showMatches(im1, im2, pers_repts1vec, repts2vec, perssfn );
	cout << "check test_pers_sift_matches_68.jpg" << endl;
#endif
	
	//pixelPts2通过透视变换得到的新的点

	cout << "apply perspective transformation matrix to pixels in " << idx << "th region." << endl;
	vector<Point2f> pers_pixelpts1vec(0);    
	cv::perspectiveTransform(pixelpts2vec, pers_pixelpts1vec, persmtx);   // pixelpts1vec = affinemtx * pixelpts2vec

	fstream perstfout("pixelspt1_transformby_pixelspt2.txt", ios::out);
	perstfout << pers_pixelpts1vec.size() << endl;
	for (int i = 0; i < pers_pixelpts1vec.size(); ++i)
	{
		perstfout << pers_pixelpts1vec[i].x << ' ' << pers_pixelpts1vec[i].y << endl;
	}
	perstfout.close();

	//由pers_pixelpts1vec求解新的mask
	Mat pers_remsk1(height, width, CV_8UC1, cv::Scalar(0));
	for (int i = 0; i < pers_pixelpts1vec.size(); ++i)
	{
		int x = pers_pixelpts1vec[i].x;
		int y = pers_pixelpts1vec[i].y;

		if (x>=width || x<0 || y>=height || y<0)
		{
			continue;
		}

		pers_remsk1.at<uchar>(y,x) = 255;
	}

	imshow("pers mask", pers_remsk1);
	waitKey(0);
	destroyWindow("pers mask");
	string persmsksfn = "perspective_mask_68_view1.jpg";
	imwrite(persmsksfn, pers_remsk1);
	
	//得到对应的source image中的区域
	Mat pers_reim1 ;
	im1.copyTo(pers_reim1, pers_remsk1);

	perssfn = folder + "perspective_region_" + type2string<int>(idx) + "_" + argv[2] + ".jpg";
	imwrite(perssfn, pers_reim1);

	string perslctsfn = folder + "perspective_lct_region_68_view5E.jpg";
	RegionColorTransfer(im1, im2, pers_remsk1, binary_remsk2, perslctsfn);
	
	cout << "done. " << endl;
	return 1;
	

	/************************求仿射变换********************************/	
	cout << endl << "affine transform: " << endl;
	Mat affinemtx  =  estimateRigidTransform(repts2vec, repts1vec, true);  //求解的变换只限制于旋转，平移，缩放的组合
	cout << "affinemtx: " << affinemtx << endl;

//#define RQ_TESTIFY
#ifdef RQ_TESTIFY
	cout << endl << "testify affine matrix is right or not." << endl;
	vector<Point2f> testPts1(0), testPts2(matchcnt);
	copy(repts2vec.begin(), repts2vec.end(), testPts2.begin());
	transform(testPts2, testPts1, affinemtx);

	fstream tfout("repts1_transformby_repts2.txt", ios::out);
	tfout << matchcnt << endl;
	for (int i = 0; i < matchcnt; ++i)
	{
		Point2f tpt1 = testPts1[i];
		tfout << tpt1.x << ' ' << tpt1.y << endl;
	}
	tfout.close();
	showMatches(im1, im2, testPts1, repts2vec, "test_sift_matches_68.jpg");
	cout << "check test_sift_matches_68.jpg" << endl;

	tfout.open("affinemtx_68.txt", ios::out);
	tfout << affinemtx << endl;
	tfout.close();
#endif

	
	//pixelPts2通过仿射变换得到的新的点

	cout << "apply affine transformation matrix to pixels in " << idx << "th region." << endl;
	vector<Point2f> pixelpts1vec(0);    
	cv::transform(pixelpts2vec, pixelpts1vec, affinemtx);   // pixelpts1vec = affinemtx * pixelpts2vec

	tfout.open("pixelspt1_transformby_pixelspt2.txt", ios::out);
	tfout << pixelpts1vec.size() << endl;
	for (int i = 0; i < pixelpts1vec.size(); ++i)
	{
		tfout << pixelpts1vec[i].x << ' ' << pixelpts1vec[i].y << endl;
	}
	tfout.close();
	
	//由pixelpts1vec求解新的mask
	Mat affine_remsk1(height, width, CV_8UC1, cv::Scalar(0));
	for (int i = 0; i < pixelpts1vec.size(); ++i)
	{
		int x = pixelpts1vec[i].x;
		int y = pixelpts1vec[i].y;

		if (x>=width || x<0 || y>=height || y<0)
		{
			continue;
		}

		affine_remsk1.at<uchar>(y,x) = 255;
	}

	imshow("affine mask", affine_remsk1);
	waitKey(0);
	destroyWindow("affine mask");
	string msksfn = "affine_mask_68_view1.jpg";
	imwrite(msksfn, affine_remsk1);
	
	//得到对应的source image中的区域
	Mat affine_reim1 ;
	im1.copyTo(affine_reim1, affine_remsk1);
	
	string affinesfn = folder + "affine_region_" + type2string<int>(idx) + "_" + argv[2] + ".jpg";
	imwrite(affinesfn, affine_reim1);

	//保存原本在target image中的区域
	
	string lctsfn = folder + "lct_region_68_" + argv[3] + ".jpg";
	RegionColorTransfer(im1, im2, affine_remsk1, binary_remsk2, lctsfn);

	cout << "\n\tdone" << endl;
	return 1;
}