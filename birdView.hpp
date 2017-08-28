#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;
using namespace std;

// calculate correspondence point for every input
// 0: left up  1: right up  2:rigth down  3: left down
class BirdView
{
public:
	BirdView(const char* configFile = NULL)
	{
		SourcePoint_OK=ParamSet_OK = false;
		maskHeigth = clickCount = camID = 0;
		targetPoint.resize(4);
		sourcePoint.resize(4);

		
		for (int i = 0;i < 4; i++)
		{
			targetPoint[i].resize(4);
			sourcePoint[i].resize(4);
		}

		// check if config file exist.
		if (configFile)
		{
			readConfig(configFile);
		}
	}
	void setInternalShift(int W, int H)
	{
		ShiftAdjust = Size(W,H);
		ParamSet_OK = false;
		setParam();
	}
	void setShift(int W, int H)
	{
		Shift = Size(W,H);
		ParamSet_OK = false;
		setParam();
	}
	void setCarSize(int W,int H)
	{
		carSize = Size(W, H);
		ParamSet_OK = false;
		setParam();
	}
	void setChessSize(int width)
	{
		chessBordWidth.width = chessBordWidth.height = width;
		ParamSet_OK = false;
		setParam();
	}
	void setMaskHeigth(int maskHeigth_)
	{
		maskHeigth = maskHeigth_;
		ParamSet_OK = false;
		setParam();
	}
	void sourcePointClick(Mat *v)
	{
		setParam(1);
		// click corner-points and record them 
		printf("cam: %d ,pointID: %d  ", camID, clickCount);
		const char *windowsName = "Source point set";
		namedWindow(windowsName);
		setMouseCallback(windowsName,on_MouseHandle, (void*)this);

		for (camID = 0, clickCount = 0; camID<4;)
		{	
			for (int i = 0; i < sourcePoint[camID].size(); i++)
			{
				circle(v[camID], sourcePoint[camID][i], 5, Scalar(255, 0, 0), 2);
			}
			imshow(windowsName, v[camID]);
			if (waitKey(20) == 'j')	break;
		}
		setMouseCallback(windowsName, NULL, NULL);
		destroyWindow(windowsName);
		saveConfig("config.yml");/*save source's points*/
		SourcePoint_OK = true;
	}
	void sourcePointClick(cv::VideoCapture *v)
	{
		setParam(1);
		Mat ans;
		// click corner-points and record them 
		printf("cam: %d ,pointID: %d  ", camID, clickCount);
		const char *windowsName = "Source point set";
		namedWindow(windowsName);
		setMouseCallback(windowsName,on_MouseHandle, (void*)this);
		for (int i =0;i<4;i++)
		{
			sourcePoint[i].clear();
		}
		for (camID = 0, clickCount = 0; camID<4;)
		{
			v[camID] >> ans;
			for (int i = 0; i < sourcePoint[camID].size(); i++)
			{
				circle(ans, sourcePoint[camID][i], 5, Scalar(255, 0, 0), 2);
			}
			imshow(windowsName, ans);
			if (waitKey(20) == 'j')	break;
		}
		setMouseCallback(windowsName, NULL, NULL);
		destroyWindow(windowsName);
		saveConfig("config.yml");/*save source's points*/
		SourcePoint_OK = true;
	}
	void saveConfig(const char* configFile = "config.yml")
	{
		for (int i = 0;i < 4; i++)
		{
			if (sourcePoint[i].empty())
			{
				std::cout << "[ERROR] sourcePoint has not been comfired all\n"<<std::endl;
				return ;
			}
		}
		FileStorage fs(configFile, FileStorage::WRITE);
		if (fs.isOpened())
		{
			for (int i = 0; i < 4; i++)
			{
				for (int k = 0; k < 4; k++)
				{
					char buf[20];
					sprintf(buf, "sourcePoint%d%d", i, k);
					write(fs, buf, sourcePoint[i][k]);
				}
			}
			fs.release();
			std::cout << "\n param save complete! \n\n";
		}
	}
	void readConfig(const char* configFile = "config.yml")
	{
		FileStorage fs(configFile, FileStorage::READ);
		if (fs.isOpened())
		{
			for (int i = 0; i < 4; i++)
			{
				for (int k = 0; k < 4; k++)
				{
					char buf[20];
					sprintf(buf, "sourcePoint%d%d", i, k);
					fs[buf] >> sourcePoint[i][k];
				}
			}
			SourcePoint_OK = true;  // source point reading completed
			ParamSet_OK = false; // setting parma 
			setParam();
			std::cout << "[WARNING] Config file read sucessfully!\n";
		}
		else  std::cout << "[WARNING] There is not a config file in folder\n";
	}
	Mat  transformView(Mat* v)
	{
		if (!SourcePoint_OK)
		{
			throw "[ERROR] Source Points have not been pointed! please Add function sourcePointClick to get Source Points!\n";
		}
		if (!ParamSet_OK)
		{
			setParam();
		}

		Mat b[4];
		Mat m = Mat(mSize, CV_8UC3 );
		int seq[4] = { 0,2,1,3 };
		for (int i = 0; i < 4; i++)
		{
			warpPerspective(v[seq[i]], b[seq[i]], Birdtransform[seq[i]], mSize);
			switch (seq[i])
			{
			case 1:
				b[seq[i]](r[seq[i]]).copyTo(m(r[seq[i]]), maskF);
				break;
			case 3:
				b[seq[i]](r[seq[i]]).copyTo(m(r[seq[i]]), maskB);
				break;
			default:
				b[seq[i]](r[seq[i]]).copyTo(m(r[seq[i]]));
				break;
			}
		}
		//drawing target points
		const Scalar color[4] = { Scalar(255,0,0),Scalar(0,255,0), Scalar(255,255,0), Scalar(0,255,255) };
		for (int i = 0; i < 16; i++) circle(m, targetPoint[i / 4][i % 4], 5, color[i / 4], 5);
		return m;
	}
	friend void on_MouseHandle(int e, int x, int y, int flag, void* param)
	{
		BirdView & birdView = *(BirdView*)param;
		int camID = birdView.camID;
		switch (e)
		{
		case EVENT_LBUTTONUP:
		{
			birdView.sourcePoint[birdView.camID][birdView.clickCount] = Point2f(x, y)	;
			printf("x:%d y:%d\n", x, y);
			birdView.clickCount++;
			if (birdView.clickCount> 3)
			{
				birdView.clickCount = 0;
				birdView.Birdtransform[camID] = getPerspectiveTransform(birdView.sourcePoint[camID], birdView.targetPoint[camID]);
				birdView.camID++;
			}
			if (birdView.camID<3)
			{
				printf("cam: %d ,pointID: %d  ", birdView.camID, birdView.clickCount);
			}
			else printf("\n");
		}
		default: break;
		}
	}

private:
	Rect r[4];
	int clickCount, camID, maskHeigth;
	Mat Birdtransform[4],maskF, maskB;
	vector<vector<Point2f>> targetPoint, sourcePoint;
	Size ShiftAdjust, Shift, chessBordWidth, mSize, carSize; //内部偏移、外部偏移
	bool SourcePoint_OK,ParamSet_OK;
	void setParam(bool tranformCheck = false)
	{
		 //// WARMING will show when Transform is running but not all parameters have been set
		if (Shift.area()== 0)
		{
			if (tranformCheck)std::cout << "[WARMING] Shift has not been set! Default value will be used" << std::endl;
			Shift.width = Shift.height = 200;
		}
		if (chessBordWidth.area() == 0)
		{
			if (tranformCheck)std::cout << "[WARMING] chessBordWidth has not been set! Default value will be used" << std::endl;
			chessBordWidth.width = chessBordWidth.height = 60;
		}
		if (ShiftAdjust.area() == 0)
		{
			if (tranformCheck)std::cout << "[WARMING] ShiftAdjust has not been set! Default value will be used" << std::endl;
			ShiftAdjust.width = ShiftAdjust.height = 30;
		}
		if (carSize.area() == 0)
		{
			if (tranformCheck)std::cout << "[WARMING] carSize has not been set! Default value will be used" << std::endl;
			carSize = Size(240, 380);
		}
		if (maskHeigth >=100 || maskHeigth <=0)
		{
			if (tranformCheck)std::cout << "[WARMING] maskHeigth has not been set! Default value will be used" << std::endl;
			maskHeigth = 200;
		}
		if (!ParamSet_OK)
		{
			/*The size of the entire output image*/
			mSize = Size(Shift.width * 2 + carSize.width + chessBordWidth.width * 2,
						 Shift.height * 2 + carSize.height + chessBordWidth.height * 2);
			/*make targetPoint, need chessBordWidth,mSize,Shift*/
			/*left*/
			targetPoint[0][0] = (Point2f(Shift.width + chessBordWidth.width, Shift.height));
			targetPoint[0][1] = (Point2f(Shift.width + chessBordWidth.width, mSize.height - Shift.height));
			targetPoint[0][2] = (Point2f(Shift.width, mSize.height - Shift.height));
			targetPoint[0][3] = (Point2f(Shift.width, Shift.height));

			/*forward*/
			targetPoint[1][0] = (Point2f(mSize.width - Shift.width, Shift.height + chessBordWidth.height));
			targetPoint[1][1] = (Point2f(Shift.width, Shift.height + chessBordWidth.height));
			targetPoint[1][2] = (Point2f(Shift.width, Shift.height));
			targetPoint[1][3] = (Point2f(mSize.width - Shift.width, Shift.height));

			/*backward*/
			targetPoint[3][0] = (Point2f(Shift.width, mSize.height - Shift.height - chessBordWidth.height));
			targetPoint[3][1] = (Point2f(mSize.width - Shift.width, mSize.height - Shift.height - chessBordWidth.height));
			targetPoint[3][2] = (Point2f(mSize.width - Shift.width, mSize.height - Shift.height));
			targetPoint[3][3] = (Point2f(Shift.width, mSize.height - Shift.height));
			/*right*/
			targetPoint[2][0] = (Point2f(mSize.width - Shift.width - chessBordWidth.width, Shift.height));
			targetPoint[2][1] = (Point2f(mSize.width - Shift.width - chessBordWidth.width, mSize.height - Shift.height));
			targetPoint[2][2] = (Point2f(mSize.width - Shift.width, mSize.height - Shift.height));
			targetPoint[2][3] = (Point2f(mSize.width - Shift.width, Shift.height));

			//need  Shift, chessBordWidth, ShiftAdjust, mSize
			/*roi*/
			r[0] = Rect(0, 0, Shift.width + chessBordWidth.width + ShiftAdjust.width, mSize.height);
			r[1] = Rect(0, 0, mSize.width, Shift.height + chessBordWidth.height + ShiftAdjust.height);
			r[2] = Rect(mSize.width - Shift.width - chessBordWidth.width - ShiftAdjust.width, 0, Shift.width + chessBordWidth.width + ShiftAdjust.width, mSize.height);
			r[3] = Rect(0, mSize.height - Shift.width - chessBordWidth.width - ShiftAdjust.width, mSize.width, Shift.height + chessBordWidth.height + ShiftAdjust.height);

			maskF = Mat(r[1].size(), CV_8UC1, Scalar(1));

			maskB = Mat(r[1].size(), CV_8UC1, Scalar(1));
			
			/*make mask, need r */
			vector<vector<Point>> maskVec;
			/*forward*/
			maskVec.push_back(vector<Point>());
			maskVec[0].push_back(Point(0, r[1].height));
			maskVec[0].push_back(Point(0, r[1].height - maskHeigth));
			maskVec[0].push_back(Point(r[0].width, r[1].height));
			maskVec.push_back(vector<Point>());
			maskVec[1].push_back(Point(r[1].width, r[1].height));
			maskVec[1].push_back(Point(r[1].width, r[1].height - maskHeigth));
			maskVec[1].push_back(Point(r[1].width - r[2].width, r[1].height));
			/*backward*/
			maskVec.push_back(vector<Point>());
			maskVec[2].push_back(Point(0, 0));
			maskVec[2].push_back(Point(0, maskHeigth));
			maskVec[2].push_back(Point(r[0].width, 0));
			maskVec.push_back(vector<Point>());
			maskVec[3].push_back(Point(mSize.width, 0));
			maskVec[3].push_back(Point(mSize.width, maskHeigth));
			maskVec[3].push_back(Point(mSize.width - r[2].width, 0));
			/*draw  mask*/
				
			drawContours(maskF, maskVec, 0, Scalar(0), CV_FILLED);
			drawContours(maskF, maskVec, 1, Scalar(0), CV_FILLED);
			drawContours(maskB, maskVec, 2, Scalar(0), CV_FILLED);
			drawContours(maskB, maskVec, 3, Scalar(0), CV_FILLED);
			

			for (size_t i = 0; i < 4 ; i++)
			{
				Birdtransform[i] = getPerspectiveTransform(sourcePoint[i], targetPoint[i]);
			}

			ParamSet_OK = true;
		}
	}
};



int main()
{
	Mat v[4];

	for (int i = 0; i < 4; i++)
	{
		char buf[10];
		sprintf(buf, "%d.png", i);
		v[i] = imread(buf);
	}


	BirdView b("config.yml");

	b.setCarSize(240, 380); 
	b.setChessSize(60);
	b.setMaskHeigth(200);
	b.setInternalShift(27,27);

	//b.sourcePointClick(v);

	while (1)
	{
		imshow("bird view", b.transformView(v));
		if (waitKey(20) == 27)	break;
	}
}
