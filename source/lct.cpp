#include "function.h"

//计算所有区域的均值和标准差
void computeMeanSdv(Mat labim, vector<vector<Point2i>> pxltable, int regionum, vector<Scalar>& means, vector<Scalar>& sdvs)
{
	//为每个区域生成一个mask，直接调用cv::meanStdDev
	
	int h = labim.rows;
	int w = labim.cols;

	for (int i = 0; i < regionum; ++i)
	{
		Mat tmpmsk(h, w, CV_8UC1, cv::Scalar(0));
		for (int j = 0; j < pxltable[i].size(); ++j)
		{
			Point2i pt = pxltable[i][j];
			tmpmsk.at<uchar>(pt.y, pt.x) = 255;
		}

		//string mskfn = "mask_" + type2string(i) + ".jpg";
		//imwrite(mskfn, tmpmsk);
		Scalar tmpmean, tmpsdv;
		meanStdDev(labim, tmpmean, tmpsdv, tmpmsk);
		means.push_back(tmpmean);
		sdvs.push_back(tmpsdv);
	}
}

void RegionColorTransfer(Mat im1, Mat im2, Mat remsk1, Mat remsk2, Mat& new_reim2, string lctsfn)
{
	int w = im1.cols;
	int h = im1.rows;

	Mat reim1 , reim2;
	im1.copyTo(reim1, remsk1);
	im2.copyTo(reim2, remsk2);

	Mat lab_im1, lab_im2;
	cvtColor(reim1, lab_im1, CV_BGR2Lab);
	cvtColor(reim2, lab_im2, CV_BGR2Lab);

	Scalar mean1, stddv1;
	Scalar mean2, stddv2;

	meanStdDev(lab_im1, mean1, stddv1, remsk1);
	meanStdDev(lab_im2, mean2, stddv2, remsk2);

	cout << "1: " << mean1 << " " << stddv1 << endl;
	cout << "2: " << mean2 << " " << stddv2 << endl;

	double Lfactor = stddv1.val[0] / stddv2.val[0];
	double Afactor = stddv1.val[1] / stddv2.val[1];
	double Bfactor = stddv1.val[2] / stddv2.val[2];

	cout << "Lfactor = " << Lfactor << endl;
	cout << "Afactor = " << Afactor << endl;
	cout << "Bfactor = " << Bfactor << endl;

	Mat lab_lctim2 = lab_im2.clone();    //Mat lab_lctim2(h, w, CV_8UC3, Scalar(0,0,0));	
	int cnt = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (remsk2.at<uchar>(y,x) != 255)
				continue;
			
			Vec3b color = lab_im2.at<Vec3b>(y,x);
			cnt ++;

			double newcolorL = Lfactor * (color[0] - mean2.val[0]) + mean1.val[0];
			double newcolorA = Afactor * (color[1] - mean2.val[1]) + mean1.val[1];
			double newcolorB = Bfactor * (color[2] - mean2.val[2]) + mean1.val[2];

			lab_lctim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,newcolorL), 255.0);
			lab_lctim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,newcolorA), 255.0);
			lab_lctim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,newcolorB), 255.0);
		}
	}
	cout << endl << "region color transfer. " << cnt << " pixels." << endl;
	
	cvtColor(lab_lctim2, new_reim2, CV_Lab2BGR);

	if (lctsfn != "")
	{
		imwrite(lctsfn, new_reim2);
	}	
}

#define WEIGHTED_LCT

void LocalColorTransfer(Mat im1, Mat im2, vector<vector<Point2f>>& pixelTable1, vector<vector<Point2f>>& pixelTable2, string folder, string imfn)
{  
	int height = im1.rows;
	int width  = im1.cols;

	vector<int> matchedRegionsIdx(0);								// 匹配区域的index	
	vector<Scalar> means1(0), means2(0), stddvs1(0), stddvs2(0);    // 匹配区域的均值和标准差
	vector<double> Lfactors(0), Afactors(0), Bfactors(0);           // 匹配区域的标准差比值

	Mat labim1, labim2;
	cvtColor(im1, labim1, CV_BGR2Lab);
	cvtColor(im2, labim2, CV_BGR2Lab);

	for (int i = 0; i < pixelTable1.size(); ++i)
	{
		if (pixelTable1[i].size() == 0)
			continue;

		int idx = i;
		matchedRegionsIdx.push_back(idx);
					
		Mat remsk1, remsk2;
		maskFromPixels(pixelTable1[idx], height, width, remsk1);
		maskFromPixels(pixelTable2[idx], height, width, remsk2);

		//第i个求得匹配的区域的均值和标准差
		Scalar mean1, stddv1, mean2, stddv2;
						
		meanStdDev(labim1, mean1, stddv1, remsk1);
		meanStdDev(labim2, mean2, stddv2, remsk2);
				
		means1.push_back(mean1);	
		stddvs1.push_back(stddv1);
		means2.push_back(mean2);
		stddvs2.push_back(stddv2);

		double lfactor = stddv1.val[0] / stddv2.val[0];
		double afactor = stddv1.val[1] / stddv2.val[1];
		double bfactor = stddv1.val[2] / stddv2.val[2];

		Lfactors.push_back(lfactor);
		Afactors.push_back(afactor);
		Bfactors.push_back(bfactor);
	}

	//保存每个匹配区域的means, stddvs
	string fn1 = folder + "means_stddvs_1.txt";
	string fn2 = folder + "means_stddvs_2.txt";
	string factorfn = folder + "lab_factors.txt";
			
	fstream fout1(fn1, ios::out); fout1 << matchedRegionsIdx.size() << endl;
	fstream fout2(fn2, ios::out); fout2 << matchedRegionsIdx.size() << endl;
	fstream ffout(factorfn, ios::out);  ffout << matchedRegionsIdx.size() << endl;
	for (int i = 0; i < matchedRegionsIdx.size(); ++i)
	{
		int idx = matchedRegionsIdx[i];	
		
		fout1 << idx << ": " << means1[i] << " " << stddvs1[i] << endl;
		fout2 << idx << ": " << means2[i] << " " << stddvs2[i] << endl;
		ffout << idx << ": " << Lfactors[i] << " " << Afactors[i] << " " << Bfactors[i] << endl;
	}
	fout1.close();
	fout2.close();


	//进行颜色变换
	Mat newim2(height, width, CV_8UC3, cv::Scalar(0,0,0));
	Mat lab_newim2(height, width, CV_8UC3, cv::Scalar(0,0,0));
	string savefn;

#ifdef  WEIGHTED_LCT
	
	double alpha = 20;
	double alpha2 = alpha * alpha;
		
	for (int y = 0; y < height; ++ y)
	{
		for (int x = 0; x < width; ++ x)
		{
			Vec3b color = labim2.at<Vec3b>(y,x);     //原本的颜色
					 
			double weightLsum = 0.0, weightAsum = 0.0, weightBsum = 0.0;
			Scalar colorsum (0.0, 0.0, 0.0);
		
			//所有区域的加权平均
		
			for (int i = 0; i < matchedRegionsIdx.size(); ++i)
			{				
				//e(-(x*x)/2*(alpha*alpha))
				double weightL = exp(-0.5 *  (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]) / alpha2 );
				double weightA = exp(-0.5 *  (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]) / alpha2 );
				double weightB = exp(-0.5 *  (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]) / alpha2 );

				//-3*x*x
				/*double weightL = exp(-3 *  (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]));
				double weightA = exp(-3 *  (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]));
				double weightB = exp(-3 *  (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]));*/
		
				weightLsum += weightL;
				weightAsum += weightA;
				weightBsum += weightB;
									
				double newcolorL = weightL * ( Lfactors[i] * (color[0] - means2[i].val[0]) + means1[i].val[0] );
				double newcolorA = weightA * ( Afactors[i] * (color[1] - means2[i].val[1]) + means1[i].val[1] );
				double newcolorB = weightB * ( Bfactors[i] * (color[2] - means2[i].val[2]) + means1[i].val[2] );
		
				colorsum.val[0] += newcolorL;
				colorsum.val[1] += newcolorA;
				colorsum.val[2] += newcolorB;
			}
		
			if (weightLsum != 0)
				lab_newim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,colorsum.val[0]/weightLsum), 255.0);
			if (weightAsum != 0)
				lab_newim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,colorsum.val[1]/weightAsum), 255.0);
			if (weightBsum != 0)
				lab_newim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,colorsum.val[2]/weightBsum), 255.0);				
		}
	}
		
	cvtColor(lab_newim2, newim2, CV_Lab2BGR);
		
	savefn = folder + "lct_weighted_" + imfn + ".jpg";   //local_weighted_view5E.jpg， 只考虑有匹配的区域进行加权平均（本文的思路）
	cout << "save " << savefn << endl;
	/*imshow("newim2", newim2);
	  waitKey(0);
	  destroyWindow("newim2");*/
	imwrite(savefn, newim2);	
#endif
	
	lab_newim2 = Mat::zeros(height, width, CV_8UC3);
	for (int i = 0; i < matchedRegionsIdx.size(); ++i)
	{
		int idx = matchedRegionsIdx[i];

		Scalar mean1 = means1[i];
		Scalar mean2 = means2[i];
		
		double lfactor = Lfactors[i];
		double afactor = Afactors[i];
		double bfactor = Bfactors[i];		

		for (int j = 0; j < pixelTable2[idx].size(); ++j)
		{
			Point2f pt = pixelTable2[idx][j];
			int y = pt.y;
			int x = pt.x;

			Vec3b color = labim2.at<Vec3b>(y,x);
			double newcolorL = lfactor * (color[0] - mean2.val[0]) + mean1.val[0];
			double newcolorA = afactor * (color[1] - mean2.val[1]) + mean1.val[1];
			double newcolorB = bfactor * (color[2] - mean2.val[2]) + mean1.val[2];

			lab_newim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,newcolorL), 255.0);
			lab_newim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,newcolorA), 255.0);
			lab_newim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,newcolorB), 255.0);
		}
	}

	cvtColor(lab_newim2, newim2, CV_Lab2BGR);
	savefn = folder + "lct_" + imfn + ".jpg";      //lct_view5E.jpg:  有匹配的区域的一对一变换
	cout << "save " << savefn << endl;
	/*imshow("newim2", newim2);
      waitKey(0);
	  destroyWindow("newim2");*/
	imwrite(savefn, newim2);
	
	cout << endl << "/*******************weighted local color transfer done******************/" << endl;
}

//每个区域都有均值和标准差，无匹配区域使用global的值
void LocalColorTransfer2(Mat im1, Mat im2, vector<vector<Point2f>>& pixelTable1, vector<vector<Point2f>>& pixelTable2, string folder, string imfn)
{
	int width = im1.cols;
	int height = im1.rows;

	Mat labim1, labim2;
	Scalar global_mean1, global_stddv1;
	Scalar global_mean2, global_stddv2;

	cvtColor(im1, labim1, CV_BGR2Lab);
	cvtColor(im2, labim2, CV_BGR2Lab);
	meanStdDev(labim1, global_mean1, global_stddv1);
	meanStdDev(labim2, global_mean2, global_stddv2);

	int regionum = pixelTable1.size();
	vector<Scalar> means1(0), means2(0), stddvs1(0), stddvs2(0);    // 匹配区域的均值和标准差
	vector<double> Lfactors(0), Afactors(0), Bfactors(0); 

	for (int i = 0; i < regionum; ++i)
	{
		if (pixelTable1[i].size() == 0)
		{
			means1.push_back(global_mean1);
			means2.push_back(global_mean2);
			stddvs1.push_back(global_stddv1);
			stddvs2.push_back(global_stddv2);
		}
		else
		{
			Mat remsk1, remsk2;
			maskFromPixels(pixelTable1[i], height, width, remsk1);
			maskFromPixels(pixelTable2[i], height, width, remsk2);

			//第i个求得匹配的区域的均值和标准差
			Scalar mean1, stddv1, mean2, stddv2;

			meanStdDev(labim1, mean1, stddv1, remsk1);
			meanStdDev(labim2, mean2, stddv2, remsk2);

			means1.push_back(mean1);	
			stddvs1.push_back(stddv1);
			means2.push_back(mean2);
			stddvs2.push_back(stddv2);
		}

		double lfactor = stddvs1[i].val[0] / stddvs2[i].val[0];
		double afactor = stddvs1[i].val[1] / stddvs2[i].val[1];
		double bfactor = stddvs1[i].val[2] / stddvs2[i].val[2];

		Lfactors.push_back(lfactor);
		Afactors.push_back(afactor);
		Bfactors.push_back(bfactor);
	}

	//进行颜色变换
	Mat newim2(height, width, CV_8UC3, cv::Scalar(0,0,0));
	Mat lab_newim2(height, width, CV_8UC3, cv::Scalar(0,0,0));
	string savefn;

#ifdef  WEIGHTED_LCT
	double alpha = 20;
	double alpha2 = alpha * alpha;

	for (int y = 0; y < height; ++ y)
	{
		for (int x = 0; x < width; ++ x)
		{
			Vec3b color = labim2.at<Vec3b>(y,x);     //原本的颜色

			double weightLsum = 0.0, weightAsum = 0.0, weightBsum = 0.0;
			Scalar colorsum (0.0, 0.0, 0.0);

			//所有区域的加权平均

			for (int i = 0; i < regionum; ++i)
			{				
				//e(-(x*x)/2*(alpha*alpha))
				double weightL = exp(-0.5 *  (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]) / alpha2 );
				double weightA = exp(-0.5 *  (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]) / alpha2 );
				double weightB = exp(-0.5 *  (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]) / alpha2 );

				//-3*x*x
				/*double weightL = exp(-3 *  (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]));
				  double weightA = exp(-3 *  (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]));
				  double weightB = exp(-3 *  (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]));*/

				weightLsum += weightL;
				weightAsum += weightA;
				weightBsum += weightB;

				double newcolorL = weightL * ( Lfactors[i] * (color[0] - means2[i].val[0]) + means1[i].val[0] );
				double newcolorA = weightA * ( Afactors[i] * (color[1] - means2[i].val[1]) + means1[i].val[1] );
				double newcolorB = weightB * ( Bfactors[i] * (color[2] - means2[i].val[2]) + means1[i].val[2] );

				colorsum.val[0] += newcolorL;
				colorsum.val[1] += newcolorA;
				colorsum.val[2] += newcolorB;
			}

			if (weightLsum != 0)
				lab_newim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,colorsum.val[0]/weightLsum), 255.0);
			if (weightAsum != 0)
				lab_newim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,colorsum.val[1]/weightAsum), 255.0);
			if (weightBsum != 0)
				lab_newim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,colorsum.val[2]/weightBsum), 255.0);				
		}
	}

	cvtColor(lab_newim2, newim2, CV_Lab2BGR);

	savefn = folder + "lct_weighted_" + imfn + "_2.jpg";       //lct_weighted_view5E_2.jpg ：  所有区域的加权平均，无匹配区域使用global_means, global_stddvs 
	cout << "save " << savefn << endl;
	/*imshow("newim2", newim2);
	waitKey(0);
	destroyWindow("newim2");*/
	imwrite(savefn, newim2);	
#endif

	lab_newim2 = Mat::zeros(height, width, CV_8UC3);
	for (int i = 0; i < regionum; ++i)
	{
		Scalar mean1 = means1[i];
		Scalar mean2 = means2[i];
		
		double lfactor = Lfactors[i];
		double afactor = Afactors[i];
		double bfactor = Bfactors[i];		

		for (int j = 0; j < pixelTable2[i].size(); ++j)
		{
			Point2f pt = pixelTable2[i][j];
			int y = pt.y;
			int x = pt.x;

			Vec3b color = labim2.at<Vec3b>(y,x);
			double newcolorL = lfactor * (color[0] - mean2.val[0]) + mean1.val[0];
			double newcolorA = afactor * (color[1] - mean2.val[1]) + mean1.val[1];
			double newcolorB = bfactor * (color[2] - mean2.val[2]) + mean1.val[2];

			lab_newim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,newcolorL), 255.0);
			lab_newim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,newcolorA), 255.0);
			lab_newim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,newcolorB), 255.0);
		}
	}

	cvtColor(lab_newim2, newim2, CV_Lab2BGR);
	savefn = folder + "lct_" + imfn + "_2.jpg";           //lct_view5E_2.jpg ： 所有区域的一对一变换
	cout << "save " << savefn << endl;
	/*imshow("newim2", newim2);
	waitKey(0);
	destroyWindow("newim2");*/
	imwrite(savefn, newim2);
}


//为每个区域计算一张CIM图像： 像素数目大于1000才进行计算， pixeltable用来进行判断
//只计算Luminance通道的CIM
//improvation : 修改公式
void computeCIMs(Mat labim, int regionum, vector<Scalar> means, vector<vector<Point2i>> pixeltable)
{
	int h = labim.rows;
	int w = labim.cols;
	
	float alpha = 20.0f;
	float alpha2 = alpha * alpha;
	for (int i = 0; i < regionum; ++ i)
	{
		Scalar imean = means[i];
		Mat cimL(h,w,CV_32FC1);
		Mat cimA(h,w,CV_32FC1);
		Mat cimB(h,w,CV_32FC1);
		float maxL = 0.0f;
		float maxA = 0.0f, maxB = 0.0f;

		for (int y = 0; y < h; ++ y)
		{
			for (int x = 0; x < w; ++ x)
			{
				Vec3b tcolor = labim.at<Vec3b>(y,x);
				float wL = exp(-0.5 * (tcolor[0] - imean.val[0]) * (tcolor[0] - imean.val[0]) / alpha2 );
				float wA = exp(-0.5 * (tcolor[1] - imean.val[1]) * (tcolor[1] - imean.val[1]) / alpha2);
				float wB = exp(-0.5 * (tcolor[2] - imean.val[2]) * (tcolor[2] - imean.val[2]) / alpha2);

				cimL.at<float>(y,x) = wL;
				cimA.at<float>(y,x) = wA;
				cimB.at<float>(y,x) = wB;

				if (wL > maxL) maxL = wL;
				if (wA > maxA) maxA = wA;
				if (wB > maxB) maxB = wB;
			}
		}
		
		Mat tcimL, tcimA, tcimB;
		cimL.convertTo(tcimL, CV_32FC1, 1.0/maxL);
		cimA.convertTo(tcimA, CV_32FC1, 1.0/maxA);
		cimB.convertTo(tcimB, CV_32FC1, 1.0/maxB);

		cout << i << "-th region computation done." << endl;

		Mat showcimL, showcimA, showcimB;
		tcimL.convertTo(showcimL, CV_8UC1, 255);
		tcimA.convertTo(showcimA, CV_8UC1, 255);
		tcimB.convertTo(showcimB, CV_8UC1, 255);

		string Lfn = "LCIM_" + type2string<int>(i) + ".jpg";
		string Afn = "ACIM_" + type2string<int>(i) + ".jpg";
		string Bfn = "BCIM_" + type2string<int>(i) + ".jpg";

		imwrite(Lfn, showcimL);
		imwrite(Afn, showcimA);
		imwrite(Bfn, showcimB);
	}

}