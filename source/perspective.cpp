#include "function.h"

//对 points1 进行透视变换 得到 points2
void perspectivePoints(vector<Point2f> srcpts, Mat persmtx,  vector<Point2f>& dstpts)
{  
	int cnt = srcpts.size();  
	double pers[3][3];
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			pers[i][j] = persmtx.at<double>(i,j);
			cout << pers[i][j] << ' ' ;
		}
		cout << endl;
	}

	for (int i = 0; i < cnt; ++i)
	{  
		float x = srcpts[i].x;  
		float y = srcpts[i].y;

		float denominator = pers[0][2] * x + pers[1][2] * y + pers[2][2];  
		
		Point2f pt; 
		pt.x = (pers[0][0] * x + pers[1][0] * y + pers[2][0]) / denominator;  
		pt.y = (pers[0][1] * x + pers[1][1] * y + pers[2][1]) / denominator;  

		dstpts.push_back(pt);
	}
	cout << dstpts.size() << " points." << endl; 

} 