#include "function.h"

//BGR -> HSI
void  ImageBGR2HSI(Mat RGBim, Mat& HSI_Him, Mat& HSI_Sim, Mat& HSI_Iim)
{
	int width = RGBim.cols;
	int height = RGBim.rows;
	
	vector<Mat> mv(3);
	split(RGBim, mv);

	Mat Bim = mv[0].clone();
	Mat Gim = mv[1].clone();
	Mat Rim = mv[2].clone();
	
	HSI_Him.create(height, width, CV_32FC1);
	HSI_Sim.create(height, width, CV_32FC1);
	HSI_Iim.create(height, width, CV_32FC1);

	//计算H,S,I分量
	for(int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			float blue = Bim.at<uchar>(y,x);
			float green = Gim.at<uchar>(y,x);
			float red = Rim.at<uchar>(y,x);

			float hsi_h, hsi_s, hsi_i;
			
			//h: [0~360]
			float theta ;
			float fenzi  = (red-green + red-blue)/2;  //numerator
			float fenmu2 = (red-green)*(red-green) + (red-blue)*(green-blue);
			float fenmu  = sqrt( (red-green)*(red-green) + (red-blue)*(green-blue) ); //dominator

			if (fenmu != 0)
			{
				theta = acos(fenzi / fenmu) * 180 / _PI;
				
				if (blue <= green)
				{
					hsi_h = theta;
				}
				else
				{
					hsi_h = 360 - theta;
				}
			}
			else
				hsi_h = 0;
		
			//s: [0~1]
			hsi_s = 1 - 3 * min(min(red,green),blue)/(red+green+blue);
			
			//i: [0~1]
			hsi_i = ((red+green+blue)/3)/255.0f;
			
			HSI_Him.at<float>(y,x) = hsi_h;
			HSI_Sim.at<float>(y,x) = hsi_s;
			HSI_Iim.at<float>(y,x) = hsi_i;			
		}
	}
	
	//Hue, Saturation, Intensity 合成可以显示的 HSI-image
	Mat showHSIim(height, width, CV_8UC3);  
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int ihsi_h  = (int)(HSI_Him.at<float>(y,x) * 255 / 360);
			int ihsi_s  = (int)(HSI_Sim.at<float>(y,x) * 255);
			int ihsi_i  = (int)(HSI_Iim.at<float>(y,x) * 255);

			showHSIim.at<Vec3b>(y,x)[0] = ihsi_h;
			showHSIim.at<Vec3b>(y,x)[1] = ihsi_s;
			showHSIim.at<Vec3b>(y,x)[2] = ihsi_i;
		}
	}
	/*imshow("showhsi", showHSIim);
	waitKey(0);
	destroyAllWindows();*/
}

void  ImageHSI2BGR(Mat HSI_Him, Mat HSI_Sim, Mat HSI_Iim, Mat& RGBim)
{
	int width  = HSI_Him.cols;
	int height = HSI_Him.rows;

	Mat Bim (height, width, CV_8UC1);
	Mat Gim (height, width, CV_8UC1);
	Mat Rim (height, width, CV_8UC1);
	
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			double hsi_h = HSI_Him.at<float>(y,x);
			double hsi_s = HSI_Sim.at<float>(y,x);
			double hsi_i = HSI_Iim.at<float>(y,x);

			//转换公式参考网上
			double blue, green, red;

			if ( hsi_h < 120 && hsi_h >= 0)
			{
				hsi_h = hsi_h * _PI / 180;    //弧度表示
				
				blue = hsi_i * ( 1 - hsi_s );
				red  = hsi_i * ( 1 + (hsi_s * cos(hsi_h)) / cos(_PI/3 - hsi_h) );
				green = 3 * hsi_i - (blue + red);
			}
			else if ( hsi_h < 240 && hsi_h >= 120)
			{
				hsi_h = (hsi_h - 120) * _PI / 180;

				red   = hsi_i * (1- hsi_s);
				green = hsi_i * (1 + (hsi_s * cos(hsi_h)) / cos(_PI/3 - hsi_h) );
				blue  = 3 * hsi_i - (red + blue); 
			}
			else 
			{
				hsi_h = (hsi_h - 240) * _PI / 180;

				green = hsi_i * (1-hsi_s);
				blue  = hsi_i * (1 + (hsi_s * cos(hsi_h)) / cos(_PI/3 - hsi_h) );
				red   = 3 * hsi_i - (green + blue);
			}

			int iblue  = blue * 255;
			int igreen = green * 255;
			int ired   = red * 255;

			Bim.at<uchar>(y,x) = iblue;
			Gim.at<uchar>(y,x) = igreen;
			Rim.at<uchar>(y,x) = ired;
		}
	}

	vector<Mat> mv(3);
	Bim.copyTo(mv[0]);
	Gim.copyTo(mv[1]);
	Rim.copyTo(mv[2]);
	merge(mv, RGBim);
	/*imshow("rgb", RGBim);
	waitKey(0);
	destroyAllWindows();*/
}

void FeaturesColorCorrection(Mat im1, Mat im2, vector<vector<Point2f>>& pixelTable2, vector<vector<int>>& sfmatchTable, vector<Point2f>& sfmatchPt1, vector<Point2f>& sfmatchPt2, string folder, string imfn)
{
	string newimfn = folder + "accv2009_" + imfn + ".png";

	//颜色空间转换
	Mat im1_H, im1_S, im1_I;  //src
	Mat im2_H, im2_S, im2_I;  //dst: regions的分割

	ImageBGR2HSI(im1, im1_H, im1_S, im1_I);
	ImageBGR2HSI(im2, im2_H, im2_S, im2_I);

	//均值滤波
	Mat mean_im1_H, mean_im1_S, mean_im1_I;
	Mat mean_im2_H, mean_im2_S, mean_im2_I;

	int w = 3;
	blur(im1_H, mean_im1_H, cv::Size(w,w));
	blur(im1_S, mean_im1_S, cv::Size(w,w));
	blur(im1_I, mean_im1_I, cv::Size(w,w));
	blur(im2_H, mean_im2_H, cv::Size(w,w));
	blur(im2_S, mean_im2_S, cv::Size(w,w));
	blur(im2_I, mean_im2_I, cv::Size(w,w));

	int height = im1.rows;
	int width  = im1.cols;

	//为每个区域计算一个变换因子, 无匹配区域则是整体特征点

	int regionum = pixelTable2.size();
	vector<float> DeltaH(0), DeltaS(0), DeltaI(0);
	
	float global_delath = 0;
	float global_deltas = 0;
	float global_deltai = 0;
	int   global_matchcnt = 0;

	for (int i = 0; i < regionum; ++i)
	{
		//当前区域的特征点
		vector<Point2f> features1(0), features2(0);
		if (sfmatchTable[i].size() == 0)
		{
			DeltaH.push_back(0);
			DeltaS.push_back(0);
			DeltaI.push_back(0);
			//cout << "no features in " << i << "th region." << endl;
			continue;
		}

		int matchcnt = sfmatchTable[i].size(); 
		
		for (int j = 0; j < matchcnt; ++j)
		{
			int tidx = sfmatchTable[i][j];
			features1.push_back(sfmatchPt1[tidx]);
			features2.push_back(sfmatchPt2[tidx]);
		}

		//计算每个区域内的deltaH, deltaS, deltaI
		double deltah = 0, deltas = 0, deltai = 0;
		for (int j = 0; j < matchcnt; ++j)
		{
			Point2f pt2 = features2[j];
			Point2f pt1 = features1[j];

			deltah += mean_im2_H.at<float>(pt2.y, pt2.x) - mean_im1_H.at<float>(pt1.y, pt1.x);
			deltas += mean_im2_S.at<float>(pt2.y, pt2.x) - mean_im1_S.at<float>(pt1.y, pt1.x);
			deltai += mean_im2_I.at<float>(pt2.y, pt2.x) - mean_im1_I.at<float>(pt1.y, pt1.x);
		}
		global_delath += deltah;
		global_deltas += deltas;
		global_deltai += deltai;
		
		deltah /= matchcnt;
		deltas /= matchcnt;
		deltai /= matchcnt;

		DeltaH.push_back(deltah);
		DeltaS.push_back(deltas);
		DeltaI.push_back(deltai);

		global_matchcnt += matchcnt;
	}

	global_delath /= global_matchcnt;
	global_deltas /= global_matchcnt;
	global_deltai /= global_matchcnt;

	for(int i = 0; i < regionum; ++i)
	{
		if (sfmatchTable[i].size() == 0)
		{
			DeltaH[i] = global_delath;
			DeltaS[i] = global_deltas;
			DeltaI[i] = global_deltai;
		}
		else
			continue;
	}

	cout << DeltaH.size()  << ' ' << DeltaS.size() << ' ' << DeltaI.size() << endl;
	cout << global_delath << endl;
	cout << global_deltas << endl;
	cout << global_deltai << endl;
	

	//进行计算
	Mat new_im2_H = im2_H.clone();
	Mat new_im2_S = im2_S.clone();
	Mat new_im2_I = im2_I.clone();

	for (int i = 0; i < regionum; ++i)
	{
		int labelidx = i;
		
		float deltah = DeltaH[labelidx];
		float deltas = DeltaS[labelidx];
		float deltai = DeltaI[labelidx];

		for (int j = 0; j < pixelTable2[i].size(); ++j)
		{
			Point2f pt = pixelTable2[i][j];

			float old_h = im2_H.at<float>(pt.y, pt.x);
			float old_s = im2_S.at<float>(pt.y, pt.x);
			float old_i = im2_I.at<float>(pt.y, pt.x);

			float new_h = old_h + deltah;
			float new_s = old_s + deltas;
			float new_i = old_i + deltai;

			new_im2_H.at<float>(pt.y, pt.x) = new_h;
			new_im2_S.at<float>(pt.y, pt.x) = new_s;
			new_im2_I.at<float>(pt.y, pt.x) = new_i;
		}
	}

	Mat RGBim;
	ImageHSI2BGR(new_im2_H, new_im2_S, new_im2_I, RGBim);
	imshow("new im2", RGBim);
	waitKey(0);
	destroyAllWindows();
	imwrite(newimfn, RGBim);
}