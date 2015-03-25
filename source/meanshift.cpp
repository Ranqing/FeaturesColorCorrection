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

	//使用msImageProcessor对图像进行分割
	msImageProcessor mss;
	mss.DefineImage(byteimg, COLOR, height, width);
	mss.Segment(sigmaS, sigmaR, minregion, HIGH_SPEEDUP);
	mss.GetResults(byteimg);
	//RegionList * rl = mss.GetBoundaries();

	//获取分割结果

	//1.得到分块后每个像素所属类的标签
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

	//2. 得到分块后图像结果
	segimg.resize(sz*ch);
	for (int i = 0; i < sz * ch; i++)
	{
		segimg[i] = byteimg[i];
	}
	if (byteimg)
		delete[] byteimg;

	//3. 得到分块后边界结果
	//segbounds


	//4. 得到分块后区域数目
	regionum = numlabels+1;   //包含标签为0的区域
}