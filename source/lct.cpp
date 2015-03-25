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

void RegionColorTransfer(Mat im1, Mat im2, Mat remsk1, Mat remsk2, string lctsfn)
{
	int w = im1.cols;
	int h = im1.rows;

	Mat reim1 , reim2;
	im1.copyTo(reim1, remsk1);
	im2.copyTo(reim2, remsk2);

	Mat lab_im1, lab_im2;
	cvtColor(reim1, lab_im1, CV_BGR2Lab);
	cvtColor(reim2, lab_im2, CV_BGR2Lab);

	/*imshow("lab_reim1", lab_reim1 );
	imshow("lab_reim2", lab_reim2 );
	waitKey(0);
	destroyAllWindows();*/

	Scalar mean1, stddv1;
	Scalar mean2, stddv2;

	meanStdDev(lab_im1, mean1, stddv1, remsk1);
	meanStdDev(lab_im2, mean2, stddv2, remsk2);

	double Lfactor = stddv1.val[0] / stddv2.val[0];
	double Afactor = stddv1.val[1] / stddv2.val[1];
	double Bfactor = stddv1.val[2] / stddv2.val[2];

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
	
	Mat lctim2;
	cvtColor(lab_lctim2, lctim2, CV_Lab2BGR);
	imshow("lct_im2", lctim2);
	waitKey(0);
	destroyWindow("lct_im2");

	if (lctsfn != "")
	{
		imwrite(lctsfn, lctim2);
	}
	
}

//算法本身的计算weight 不一定 是对的
void CIMLocalColorTransfer(Mat labim1, Mat labim2, vector<Scalar> means1, vector<Scalar> sdvs1, vector<Scalar> means2, vector<Scalar> sdvs2,  Mat& lctim2)
{
	int w = labim1.cols;
	int h = labim1.rows;
	int regionum = means1.size();

	vector<double> Lfactors(0), Afactors(0), Bfactors(0);
	for (int i = 0; i < regionum; ++i)
	{
		Scalar tsdv = sdvs2[i];   //target
		Scalar ssdv = sdvs1[i];   //source

		double lfactor = (ssdv.val[0] * 1.0) / (tsdv.val[0]);
		double afactor = (ssdv.val[1] * 1.0) / (tsdv.val[1]);
		double bfactor = (ssdv.val[2] * 1.0) / (tsdv.val[2]);

		Lfactors.push_back(lfactor);
		Afactors.push_back(afactor);
		Bfactors.push_back(bfactor);
	}

	cout << "factors has been calculated." << endl;

	double alpha2 = 20.0 * 20.0;

	Mat lab_lctim2(h, w, CV_8UC3, Scalar(0,0,0));
	for (int y = 0; y < h; ++ y)
	{
		for (int x = 0; x < w; ++ x)
		{
			Vec3b color = labim2.at<Vec3b>(y,x);

			double weightLsum = 0.0, weightAsum = 0.0, weightBsum = 0.0;
			Scalar colorsum(0.0,0.0,0.0);

			//所有区域进行加权平均
			for (int i = 0; i < regionum; ++i)
			{	
				//e(-3*x*x)
				double weightL = exp(-3 * (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]) );
				double weightA = exp(-3 * (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]) );
				double weightB = exp(-3 * (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]) );	

				//e(-(x*x)/2*(alpha*alpha))
				//float weightL = exp(-0.5 *  (color.val[0] - means2[i].val[0]) * (color.val[0] - means2[i].val[0]) / alpha2 );
				//float weightA = exp(-0.5 *  (color.val[1] - means2[i].val[1]) * (color.val[1] - means2[i].val[1]) / alpha2);
				//float weightB = exp(-0.5 *  (color.val[2] - means2[i].val[2]) * (color.val[2] - means2[i].val[2]) / alpha2);

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
				lab_lctim2.at<Vec3b>(y,x)[0] = (int)min(max(0.0,colorsum.val[0]/weightLsum), 255.0);
			if (weightAsum != 0)
				lab_lctim2.at<Vec3b>(y,x)[1] = (int)min(max(0.0,colorsum.val[1]/weightAsum), 255.0);
			if (weightBsum != 0)
				lab_lctim2.at<Vec3b>(y,x)[2] = (int)min(max(0.0,colorsum.val[2]/weightBsum), 255.0);
		}
	}

	imshow("lab", lab_lctim2);
	waitKey(0);
	destroyWindow("lab");
	cvtColor(lab_lctim2, lctim2, CV_Lab2BGR);
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
		if (pixeltable[i].size() < 1000)
			continue;

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