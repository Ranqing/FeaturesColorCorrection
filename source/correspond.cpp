#include "function.h"

//利用有匹配的区域内的特征点求解出变换矩阵
//变换矩阵应用到区域内所有像素,得到对应的区域内的所有像素点
void FindRegionMapping(Mat im1, Mat im2, vector<vector<int>>& sfmatchTable, vector<Point2f>& sfmatchPts1, vector<Point2f>& sfmatchPts2, vector<vector<Point2f>>& pixelTable2, vector<vector<Point2f>>& out_pixelTable1, string folder)
{
	int width = im1.cols;
	int height = im1.rows;
	int regionum2 = pixelTable2.size();

	int matchedcnt = 0;
	
	for (int i = 0; i < regionum2; ++i)
	{
		//if (sfmatchTable[i].size() < 4)
		if (sfmatchTable[i].size() == 0)
		{
			out_pixelTable1.push_back(vector<Point2f>());
			continue;                   //can't compute a transform
		}
		
		int idx = i;
		int matchcnt = sfmatchTable[idx].size();      //区域内的匹配数目

		vector<Point2f> repts1(0), repts2(0);  
		for (int j = 0; j < matchcnt; ++j)
		{
			int tidx = sfmatchTable[idx][j];
			repts1.push_back(sfmatchPts1[tidx]);
			repts2.push_back(sfmatchPts2[tidx]);
		}

		if (matchcnt == 1 || matchcnt == 2)
		{
			cout << endl << idx << "th-region: translation" << endl;
			int deltax = 0;
			for (int j = 0; j < matchcnt; ++j)
			{
				deltax += (repts1[j].x - repts2[j].x); 
			}
			deltax /= matchcnt;
			cout << "delta x = " << deltax << endl;

			//对应的点
			vector<Point2f> pixelpts1(0);
			for (int k = 0; k < pixelTable2[idx].size(); ++k)
			{
				Point2f pt = pixelTable2[idx][k];
				Point2f newpt = Point2f(pt.x + deltax, pt.y);
				pixelpts1.push_back(newpt);
			}

			out_pixelTable1.push_back(pixelpts1);
		}

		else if (matchcnt == 3)
		{
			//affine transform
			if (idx == 48)
			{
				getchar();
				cout << matchcnt  << endl;
				cout << repts2[0]  << ' ' << repts1[0] << endl;
				cout << repts2[1]  << ' ' << repts1[1] << endl;
				cout << repts2[2]  << ' ' << repts1[2] << endl;
			}
			cout << endl << idx << "th-region: affine transform" << endl;
			
			Mat affinemtx = estimateRigidTransform(repts2, repts1, true);
			
			cout << "affine matrix: " << endl;
			cout << affinemtx << endl;
			cout << "apply affine transformation matrix to pixel in " << idx << "th region." << endl;
			
			vector<Point2f> aff_pixelpts1(0);
			cv::transform(pixelTable2[idx], aff_pixelpts1, affinemtx);
			
			out_pixelTable1.push_back(aff_pixelpts1);
		}

		else if (matchcnt >= 4)
		{
			cout << endl << idx << "th-region: perspective transform" << endl;
			
			Mat persmtx  = findHomography(repts2, repts1, RANSAC);    //source, target, method
			
			cout << "perspective matrix: " << endl;
			cout << persmtx  << endl;
			cout << "apply perspective transformation matrix to pixels in " << idx << "th region." << endl; 

			vector<Point2f> pers_pixelpts1(0);
			cv::perspectiveTransform(pixelTable2[idx], pers_pixelpts1, persmtx);
			
			out_pixelTable1.push_back(pers_pixelpts1);
		}

		Mat remsk1, remsk2;
		maskFromPixels(out_pixelTable1[idx], height, width, remsk1);
		maskFromPixels(pixelTable2[idx], height, width, remsk2);

		Mat reim1, reim2;
		im1.copyTo(reim1, remsk1);
		im2.copyTo(reim2, remsk2);
		Mat canvas(height, 2*width, CV_8UC3);
		reim1.copyTo(canvas(Rect(0,0,width,height)));
		reim2.copyTo(canvas(Rect(width,0,width,height)));

		string savefn = folder + "correspond_region_" + type2string<int>(idx) + ".jpg";
		imwrite(savefn, canvas);

		matchedcnt ++;

		//第i个求得匹配的区域的标签为idx
		//matchedRegionsIdx.push_back(idx);

		//第i个求得匹配的区域的均值和标准差
		//Scalar mean1, stddv1, mean2, stddv2;

		//meanStdDev(labim1, mean1, stddv1, remsk1);
		//meanStdDev(labim2, mean2, stddv2, remsk2);

		//means1.push_back(mean1);	
		//stddvs1.push_back(stddv1);
		//means2.push_back(mean2);
		//stddvs2.push_back(stddv2);
	}
	
	cout << endl << matchedcnt  << " matched region." << endl;

	//保存pixelTable1
	//保存pixelTable2
	savePixelTable(out_pixelTable1, folder + "pixelTable1.txt");
	savePixelTable(pixelTable2, folder + "pixelTable2.txt");

	//将pixelTable1的结果转换为labels1
	//保存labels1
	vector<int> seglabels1(height * width, -1);    
	for (int i = 0; i < out_pixelTable1.size(); ++i)
	{
		for (int j = 0; j < out_pixelTable1[i].size(); ++j)
		{
			int y = min(max((int)out_pixelTable1[i][j].y, 0), height-1);
			int x = min(max((int)out_pixelTable1[i][j].x, 0), width -1);
			seglabels1[y * width + x] = i;
		}
	}	
	saveLabels(seglabels1, width, height, folder + "labels1.txt");
	cout << "pixelTable convert to labels done." << endl;
	cout << "save labels1.txt" << endl;

	cout << endl << "/*******************find regions correspondence done******************/" << endl;

}

void FindRegionMapping(Mat im1, Mat im2, vector<vector<Point2f>>& matchTable2, vector<vector<Point2f>>& matchTable1, vector<vector<Point2f>>& pixelTable2, vector<vector<Point2f>>& out_pixelTable1, string folder)
{
	int width = im1.cols;
	int height = im1.rows;
	int regionum2 = pixelTable2.size();
	
	int matched = 0;		//已对应区域的数目
		
	for (int i = 0; i < regionum2; ++i)
	{
		if (matchTable2[i].size() == 0)
		{
			out_pixelTable1.push_back(vector<Point2f>());
			continue;                   //can't compute a transform
		}

		int idx = i;
		int matchcnt = matchTable2[idx].size();      //区域内的匹配数目

		vector<Point2f>& repts1 = matchTable1[idx];
		vector<Point2f>& repts2 = matchTable2[idx];

		if (matchcnt == 1 || matchcnt == 2 || matchcnt == 3)
		{
			cout << endl << idx << "th-region: translation" << endl;
			int deltax = 0;
			for (int j = 0; j < matchcnt; ++j)
			{
				deltax += (repts1[j].x - repts2[j].x); 
			}
			deltax /= matchcnt;
			cout << "delta x = " << deltax << endl;

			//对应的点
			vector<Point2f> pixelpts1(0);
			for (int k = 0; k < pixelTable2[idx].size(); ++k)
			{
				Point2f pt = pixelTable2[idx][k];
				Point2f newpt = Point2f(pt.x + deltax, pt.y);
				pixelpts1.push_back(newpt);
			}

			out_pixelTable1.push_back(pixelpts1);
		}

		else if (matchcnt == 3)
		{
			//affine transform
			/*if (idx == 48)
			{
			getchar();
			cout << matchcnt  << endl;
			cout << repts2[0]  << ' ' << repts1[0] << endl;
			cout << repts2[1]  << ' ' << repts1[1] << endl;
			cout << repts2[2]  << ' ' << repts1[2] << endl;
			}*/

			/*cout << endl << idx << "th-region: affine transform" << endl;


			Mat affinemtx = estimateRigidTransform(repts2, repts1, true);

			cout << "affine matrix: " << endl;
			cout << affinemtx << endl;
			cout << "apply affine transformation matrix to pixel in " << idx << "th region." << endl;

			vector<Point2f> aff_pixelpts1(0);
			cv::transform(pixelTable2[idx], aff_pixelpts1, affinemtx);

			out_pixelTable1.push_back(aff_pixelpts1);*/
		}

		else if (matchcnt >= 4)
		{
			cout << endl << idx << "th-region: perspective transform" << endl;

			Mat persmtx  = findHomography(repts2, repts1, RANSAC);    //source, target, method

			cout << "perspective matrix: " << endl;
			cout << persmtx  << endl;
			cout << "apply perspective transformation matrix to pixels in " << idx << "th region." << endl; 

			vector<Point2f> pers_pixelpts1(0);
			cv::perspectiveTransform(pixelTable2[idx], pers_pixelpts1, persmtx);

			out_pixelTable1.push_back(pers_pixelpts1);
		}

		//显示结果
		Mat remsk1, remsk2;
		maskFromPixels(out_pixelTable1[idx], height, width, remsk1);
		maskFromPixels(pixelTable2[idx], height, width, remsk2);

		Mat reim1, reim2;
		im1.copyTo(reim1, remsk1);
		im2.copyTo(reim2, remsk2);
		
		Mat canvas(height, 2*width, CV_8UC3);
		reim1.copyTo(canvas(Rect(0,0,width,height)));
		reim2.copyTo(canvas(Rect(width,0,width,height)));

		string savefn = folder + "/correspond_region_" + type2string<int>(idx) + ".jpg";
		imwrite(savefn, canvas);

		matched ++;
	}

	cout << endl << matched  << " matched region." << endl;

	//保存pixelTable1
	//保存pixelTable2
	savePixelTable(out_pixelTable1, folder + "/pixelTable1.txt");
	savePixelTable(pixelTable2, folder + "/pixelTable2.txt");

	//将pixelTable1的结果转换为labels1
	//保存labels1
	vector<int> seglabels1(height * width, -1);    
	for (int i = 0; i < out_pixelTable1.size(); ++i)
	{
		for (int j = 0; j < out_pixelTable1[i].size(); ++j)
		{
			int y = min(max((int)out_pixelTable1[i][j].y, 0), height-1);
			int x = min(max((int)out_pixelTable1[i][j].x, 0), width -1);
			seglabels1[y * width + x] = i;
		}
	}	
	saveLabels(seglabels1, width, height, folder + "/labels1.txt");
	cout << "pixelTable convert to labels done." << endl;
	cout << "save labels1.txt" << endl;

	cout << endl << "/*******************find regions correspondence done******************/" << endl;

}