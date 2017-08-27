#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;
using namespace std;

// calculate correspondence point for every input
// 0: left up  1: right up  2:rigth down  3: left down

Mat Birdtransform[4];
Point2f targetPoint[4][4], sourcePoint[4][4];
int clickCount = 0, camID = 0;

void on_MouseHandle(int e, int x, int y, int flag, void* param)
{

	Mat*v = (Mat*)param;
	switch (e)
	{
	case EVENT_LBUTTONUP:
	{
		sourcePoint[camID][clickCount] = Point2f(x, y);
		circle(v[camID], Point2f(x, y), 5, Scalar(255, 0, 0), 2);
		printf("x:%d y:%d\n", x, y);
		clickCount++;
		if (clickCount> 3)
		{
			clickCount = 0;
			Birdtransform[camID] = getPerspectiveTransform(sourcePoint[camID], targetPoint[camID]);
			camID++;
		}
		printf("cam: %d ,pointID: %d  ", camID, clickCount);
	}
	default: break;
	}
}

void save()
{
	FileStorage fs("config.yml", FileStorage::WRITE);
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
		cout << "\n param save complete! \n\n";
	}
}

bool read()
{
	FileStorage fs("config.yml", FileStorage::READ);
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
			Birdtransform[i] = getPerspectiveTransform(sourcePoint[i], targetPoint[i]);
		}
		cout << "\n param read complete! \n\n";
		return true;
	}
	cout << "\n param file doesn't exist! \n\n";
	return false;
}

int main()
{
	/**************************  setting parameters *************************/
	Mat v[4];
	int maskHeigth = 200;
	double W = 220, H = 380;
	Size chessBordWidth(60, 60), Shift(300, 300);

	for (int i = 0; i < 4; i++)
	{
		char buf[10];
		sprintf(buf, "%d.png", i);
		v[i] = imread(buf);
	}

	/*********************  calculate sizes for everything *******************/
	//
	Size   mSize(Shift.width * 2 + W + chessBordWidth.width * 2,
		Shift.height * 2 + H + chessBordWidth.height * 2);
	Mat m = Mat(mSize, v[0].type());

	/*left*/
	targetPoint[0][3] = Point2f(Shift.width, Shift.height);
	targetPoint[0][0] = Point2f(Shift.width + chessBordWidth.width, Shift.height);
	targetPoint[0][1] = Point2f(Shift.width + chessBordWidth.width, mSize.height - Shift.height);
	targetPoint[0][2] = Point2f(Shift.width, mSize.height - Shift.height);
	/*forward*/
	targetPoint[1][2] = Point2f(Shift.width, Shift.height);
	targetPoint[1][3] = Point2f(mSize.width - Shift.width, Shift.height);
	targetPoint[1][0] = Point2f(mSize.width - Shift.width, Shift.height + chessBordWidth.height);
	targetPoint[1][1] = Point2f(Shift.width, Shift.height + chessBordWidth.height);
	/*backward*/
	targetPoint[3][0] = Point2f(Shift.width, mSize.height - Shift.height - chessBordWidth.height);
	targetPoint[3][1] = Point2f(mSize.width - Shift.width, mSize.height - Shift.height - chessBordWidth.height);
	targetPoint[3][2] = Point2f(mSize.width - Shift.width, mSize.height - Shift.height);
	targetPoint[3][3] = Point2f(Shift.width, mSize.height - Shift.height);
	/*right*/
	targetPoint[2][0] = Point2f(Shift.width + chessBordWidth.width + W, Shift.height);
	targetPoint[2][3] = Point2f(mSize.width - Shift.width, Shift.height);
	targetPoint[2][2] = Point2f(mSize.width - Shift.width, mSize.height - Shift.height);
	targetPoint[2][1] = Point2f(Shift.width + chessBordWidth.width + W, mSize.height - Shift.height);

	/************  decide whether to collect new Source's point **************/

	printf("new Click??  [y/n]\n");
	char enter;
	enter = getchar();

	if (!read() || enter == 'y')
	{
		// click corner-points and record them 
		printf("cam: %d ,pointID: %d  ", camID, clickCount);
		const char *windowsName = "Source point set";
		namedWindow(windowsName);
		setMouseCallback(windowsName, on_MouseHandle, (void*)v);
		for (camID = 0, clickCount = 0; camID<4;)
		{
			imshow(windowsName, v[camID]);
			if (waitKey(20) == 'j')	break;
		}
		setMouseCallback(windowsName, NULL, NULL);
		destroyWindow(windowsName);
		save();/*save source's points*/
	}

	/**************************  combine four image  *************************/
	
	Mat b[4];
	int seq[4] = { 0,2,1,3 };
	int ShiftAdjust = 30;
	Rect r[4] =
	{
		Rect(0,0,Shift.width + chessBordWidth.width + ShiftAdjust,  mSize.height),
		Rect(0,0,mSize.width,Shift.height + chessBordWidth.height + ShiftAdjust),
		Rect(mSize.width - Shift.width - chessBordWidth.width - ShiftAdjust,0,Shift.width + chessBordWidth.width + ShiftAdjust,mSize.height),
		Rect(0,mSize.height - Shift.width - chessBordWidth.width - ShiftAdjust,mSize.width,Shift.height + chessBordWidth.height + ShiftAdjust)
	};

	Mat maskF = Mat(r[1].size(), CV_8UC1, Scalar(1));
	Mat maskS = Mat(r[1].size(), CV_8UC1, Scalar(1));
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
	maskVec[2].push_back(Point(r[0].width, 0));
	maskVec[2].push_back(Point(0, maskHeigth));
	maskVec.push_back(vector<Point>());
	maskVec[3].push_back(Point(mSize.width, 0));
	maskVec[3].push_back(Point(mSize.width, maskHeigth));
	maskVec[3].push_back(Point(mSize.width - r[2].width, 0));
	
	/*form  mask*/
	drawContours(maskF, maskVec, 0, Scalar(0), CV_FILLED);
	drawContours(maskF, maskVec, 1, Scalar(0), CV_FILLED);
	drawContours(maskS, maskVec, 2, Scalar(0), CV_FILLED);
	drawContours(maskS, maskVec, 3, Scalar(0), CV_FILLED);

	for (int i = 0; i < 4; i++)
	{
		warpPerspective(v[seq[i]], b[seq[i]], Birdtransform[seq[i]], mSize);
		switch (seq[i])
		{
		case 1:
			b[seq[i]](r[seq[i]]).copyTo(m(r[seq[i]]), maskF);
			break;
		case 3:
			b[seq[i]](r[seq[i]]).copyTo(m(r[seq[i]]), maskS);
			break;
		default:
			b[seq[i]](r[seq[i]]).copyTo(m(r[seq[i]]));
			break;
		}
	}

	//drawing target points in final image
	Scalar color[4] = { Scalar(255,0,0),Scalar(0,255,0), Scalar(255,255,0), Scalar(0,255,255) };
	for (int i = 0; i < 16; i++)
		circle(m, targetPoint[i / 4][i % 4], 5, color[i / 4], 5);

	/**************************  output birdeye  view  *************************/
	while (1)
	{
		imshow("b", m);
		if (waitKey(5) == 27)
		{
			break;
		}
	}
}
