#include "function.h"
#include "msImageProcessor.h"

void DrawContoursAroundSegments(vector<uchar>&	segmentedImage,	const int width, const int height, const int ch, const Scalar color)
{
	// Pixel offsets around the centre pixels starting from left, going clockwise
	const int dx8[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
	const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1};

	int sz = segmentedImage.size();    // w * h 
	vector<bool> istaken(sz, false);

	int mainindex  = 0;
	for( int j = 0; j < height; j++ )
	{
		for( int k = 0; k < width; k++ )
		{
			int np = 0;
			for( int i = 0; i < 8; i++ )
			{
				int x = k + dx8[i];
				int y = j + dy8[i];

				if( (x >= 0 && x < width) && (y >= 0 && y < height) )
				{
					int index = y*width + x;
					if( false == istaken[index] )
					{
						if( (int)segmentedImage[mainindex * ch] != (int)segmentedImage[index * ch] ) np++;
					}
				}
			}
			if( np > 2 )//1 for thicker lines and 2 for thinner lines
			{
				segmentedImage[(j*width + k) * ch] = color.val[0];
				segmentedImage[(j*width + k) * ch + 1] = color.val[1];
				segmentedImage[(j*width + k) * ch + 2] = color.val[2];
				istaken[mainindex] = true;
			}
			mainindex++;
		}
	}
}

//return how many regions
void DoMeanShiftSegmentation(vector<uchar> inputimg, const int width, const int height, const int ch, const int sigmaS, const float sigmaR, const int minregion, 
	vector<uchar>& segimg, vector<uchar>& segbound, vector<int>& seglabels, int& regionum)
{
	//inputimg vector<uchar> -> Byte *  : attention to RGB order

	int sz = width * height;
	BYTE * byteimg = new BYTE[sz * ch];
	for (int i = 0; i < sz * ch; ++i)
	{
		byteimg[i] = inputimg[i];
	}

	//ʹ��msImageProcessor��ͼ����зָ�
	msImageProcessor mss;
	mss.DefineImage(byteimg, COLOR, height, width);
	mss.Segment(sigmaS, sigmaR, minregion, HIGH_SPEEDUP);
	mss.GetResults(byteimg);
	//RegionList * rl = mss.GetBoundaries();

	//��ȡ�ָ���

	//1.�õ��ֿ��ÿ������������ı�ǩ
	int* p_labels = new int[sz], numlabels = 0;
	mss.GetLabels(p_labels);
	seglabels.resize(sz);
	for (int i = 0; i < sz; ++i)
	{
		seglabels[i] = p_labels[i];
		if (seglabels[i] > numlabels)
			numlabels = seglabels[i];
	}
	if (p_labels)
		delete[] p_labels;

	//2. �õ��ֿ��ͼ����
	segimg.resize(sz*ch);
	for (int i = 0; i < sz * ch; i++)
	{
		segimg[i] = byteimg[i];
	}
	if (byteimg)
		delete[] byteimg;

	//3. �õ��ֿ��߽���
	//segbounds


	//4. �õ��ֿ��������Ŀ
	regionum = numlabels+1;   //������ǩΪ0������
}

/**
 * @param src Image to segment
 * @param labels		vector<int>   where the (int) labels will be written in
 * @param segments	    vector<uchar> where the segmentation results will be written in
 * @param regioncnt	    Number of different labels
 */
// new 
void DoMeanShiftSegmentation(const Mat& src, int sigmaS, float sigmaR, int minR, vector<uchar>& segments, vector<int>& labels, int& regioncnt)
{
	msImageProcessor proc;
	proc.DefineImage(src.data, (src.channels() == 3 ? COLOR : GRAYSCALE), src.rows, src.cols);
	proc.Segment(sigmaS,sigmaR, minR, MED_SPEEDUP);//HIGH_SPEEDUP, MED_SPEEDUP, NO_SPEEDUP; high: set speedupThreshold, otherwise the algorithm uses it uninitialized!

	regioncnt = proc.GetRegionsCnt();

	//��ǩ���
	Mat labels_dst = cv::Mat(src.size(), CV_32SC1);
	proc.GetRegionsLabels(labels_dst.data);

	//meanshift�ָ�ͼ��
	Mat segment_dst = cv::Mat(src.size(), CV_8UC3);
	proc.GetResults(segment_dst.data);	

	//test
	/*imshow("segmentation", segment_dst);
	waitKey(0);
	destroyWindow("segmentation");*/

	int w = src.cols;
	int h = src.rows;
	labels.resize(w*h);
	segments.resize(w*h*3);
	for (int y = 0; y < h; ++ y)
		for (int x = 0; x < w; ++x)
		{
			labels[y*w+x] = labels_dst.at<int>(y,x);
			segments[(y*w+x)*3 + 0] = segment_dst.at<Vec3b>(y,x)[0];
			segments[(y*w+x)*3 + 1] = segment_dst.at<Vec3b>(y,x)[1];
			segments[(y*w+x)*3 + 2] = segment_dst.at<Vec3b>(y,x)[2];
		}


}