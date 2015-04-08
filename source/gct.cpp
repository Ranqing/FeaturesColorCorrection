#include "function.h"

void ColorTransfer(Mat lab_srcim, Mat lab_dstim, Mat srcmsk, Mat dstmsk, Mat& lab_newdstim)
{
	/*	
	1. 计算src和ref的均值和方差
	2. 原图的每一个像素进行线性组合
	*/
	int rows = lab_srcim.rows;
	int cols = lab_srcim.cols;
	
	Scalar src_mean, src_sdv;  
	Scalar dst_mean, dst_sdv;  

	meanStdDev(lab_srcim, src_mean, src_sdv, srcmsk);
	meanStdDev(lab_dstim, dst_mean, dst_sdv, dstmsk);

	//cout << src_mean.val[0] << ", " << src_mean.val[1] << ", " << src_mean.val[2] << endl;
	//cout << src_sdv.val[0] << ", " << src_sdv.val[1] << ", " << src_sdv.val[2] << endl << endl;

	//cout << dst_mean.val[0] << ", " << dst_mean.val[1] << ", " << dst_mean.val[2] << endl;
	//cout << dst_sdv.val[0] << ", " << dst_sdv.val[1] << ", " << dst_sdv.val[2] << endl;

	//根据整副图像计算出的transfer因子
	double Lfactor = (src_sdv.val[0] * 1.0) / dst_sdv.val[0];
	double afactor = (src_sdv.val[1] * 1.0) / dst_sdv.val[1];
	double bfactor = (src_sdv.val[2] * 1.0) / dst_sdv.val[2];

	lab_newdstim = lab_srcim.clone();

	for(int y =0 ; y<rows; y++)
	{
		for(int x=0; x<cols; x++)
		{
			if (dstmsk.at<uchar>(y, x) == 255)
			{
				double transfer0 = Lfactor * ( lab_dstim.at<Vec3b>(y, x)[0] - dst_mean.val[0]) + src_mean.val[0];
				double transfer1 = afactor * ( lab_dstim.at<Vec3b>(y, x)[1] - dst_mean.val[1]) + src_mean.val[1];
				double transfer2 = bfactor * ( lab_dstim.at<Vec3b>(y, x)[2] - dst_mean.val[2]) + src_mean.val[2];

				//lab_dstmtx.at<Vec3b>(y, x)[0] = minvalue( Lfactor * ( lab_dstmtx.at<Vec3b>(y, x)[0] - src_mean.val[0]) + ref_mean.val[0], 255.0);  // Lab空间中的L分量 
				//lab_dstmtx.at<Vec3b>(y, x)[1] = minvalue( afactor * ( lab_dstmtx.at<Vec3b>(y, x)[1] - src_mean.val[1]) + ref_mean.val[1], 255.0);  // Lab空间中的a分量
				//lab_dstmtx.at<Vec3b>(y, x)[2] = minvalue( bfactor * ( lab_dstmtx.at<Vec3b>(y, x)[2] - src_mean.val[2]) + ref_mean.val[2], 255.0);  // Lab空间中的b分量
			
				lab_newdstim.at<Vec3b>(y, x)[0] = std::min(255.0, std::max(transfer0, 0.0)); 
				lab_newdstim.at<Vec3b>(y, x)[1] = std::min(255.0, std::max(transfer1, 0.0));
				lab_newdstim.at<Vec3b>(y, x)[2] = std::min(255.0, std::max(transfer2, 0.0));
			}			
		}
	}

	/*imshow("lab_newdst", lab_newdstim);
	waitKey(0);
	destroyWindow("lab_newdst");*/
}

void GlobalColorTransfer(Mat srcim, Mat dstim, Mat& gct_dstim, string savefn)
{
	Mat lab_srcim, lab_dstim;
	cvtColor(srcim, lab_srcim, CV_BGR2Lab);
	cvtColor(dstim, lab_dstim, CV_BGR2Lab);

	Mat srcmsk (srcim.rows, srcim.cols, CV_8UC1, cv::Scalar(255));
	Mat dstmsk (dstim.rows, dstim.cols, CV_8UC1, cv::Scalar(255));

	Mat lab_newdstim;
	ColorTransfer(lab_srcim, lab_dstim, srcmsk, dstmsk, lab_newdstim);

	gct_dstim = dstim.clone();
	cvtColor(lab_newdstim, gct_dstim, CV_Lab2BGR);	

	if (savefn != "")
	{
		cout << savefn << endl;
		imwrite(savefn, gct_dstim);
	}
}