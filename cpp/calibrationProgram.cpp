#include <opencv2\opencv.hpp>
#include <iostream>
#include <mutex>

#define oo 1E9
#define CAM_WIDTH 720
#define CAM_HEIGTH 480
#define OUTPUT_WIDTH 1024
#define OUTPUT_HEIGTH 500
#define WIDTH_ADJUST 90
#define HEIGHT_ADJUST 0
using namespace std;
using namespace cv;

int camID;
//设置是否为鱼眼相机
const bool FISH_EYE = true;

const char * WINDOWSNAME = "ChessCatch";
//使用的抓取帧数目
const int   usedImageForCalib = 15;

VideoCapture v;
//棋盘点大小
Size board_size = Size(4,6);
//帧缓冲区域
Mat imageBuf; 
mutex Mutex_imageBuf;
// 任务是否完成标志
bool missionFinish; 
mutex Mutex_missionFinish;
// 是否找到棋盘标志
bool patternFind;
mutex Mutex_patternFind;

vector<Point2f> corners;
vector<vector<Point2f>>  corners_Seq;   // 所有有效图片的点集合
vector<vector<Point3f>>  object_Points; // 世界坐标集合

Mat intrinsic_matrix;   // 校正矩阵1 
Mat distortion_coeffs;  // 校正矩阵2

Mat mapx, mapy;			// 映射矩阵

void save()
{
	char buf[32];
	sprintf(buf, "data/CamCalibParma%d.yml", camID);
	FileStorage fs(buf, FileStorage::WRITE);
	if (fs.isOpened())
	{
		write(fs, "camType", (FISH_EYE? "eyeCam":"normalCam"));
		write(fs,"intrinsic_matrix",intrinsic_matrix);
		write(fs, "distortion_coeffs", distortion_coeffs);
		fs.release();
		cout << "\n param save complete! \n\n";
	}
}

bool read()
{
	char buf[32];
	sprintf(buf, "data/CamCalibParma%d.yml", camID);
	FileStorage fs(buf, FileStorage::READ);
	if (fs.isOpened())
	{
		fs["intrinsic_matrix"] >> intrinsic_matrix;
		fs["distortion_coeffs"] >> distortion_coeffs;
		/*fs["mapx"] >> mapx;
		fs["mapy"] >> mapy;*/
		Mat R = Mat::eye(3, 3, CV_32F);
		Mat newIn;
		intrinsic_matrix.copyTo(newIn);
		newIn.at<double>(0, 2) = OUTPUT_WIDTH / 2 - WIDTH_ADJUST;
		newIn.at<double>(1, 2) = OUTPUT_HEIGTH / 2 - HEIGHT_ADJUST;
		if (FISH_EYE)
			fisheye::initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, newIn, Size(OUTPUT_WIDTH,OUTPUT_HEIGTH ), CV_32FC1, mapx, mapy);
		else
			initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, newIn, Size(OUTPUT_WIDTH, OUTPUT_HEIGTH), CV_32FC1, mapx, mapy);

		cout << "\n param read complete! \n\n";
		return true;
	}
	return false;
}

void display(bool calibrationed = 0)
{
	while (1)
	{
		unique_lock<mutex> lock_1(Mutex_missionFinish), lock_2(Mutex_imageBuf);
		v >> imageBuf;
		
		if(calibrationed)
			remap(imageBuf, imageBuf, mapx, mapy, CV_INTER_LINEAR);
		imshow(WINDOWSNAME, imageBuf);
		lock_2.unlock();
		if (missionFinish)
		{
			destroyWindow(WINDOWSNAME);
			break;
		}
		lock_1.unlock();
		waitKey(2);
	}
}

void chessCheck(int num)
{
	missionFinish = false; //任务开始标志

	thread([] {display(0); }).detach(); // 用来显示实时图像

	thread([&]()
	{
		int frameFound = 0,frameNeed = num,timeCount = 0;

		while (1)
		{
			unique_lock<mutex> lock_1(Mutex_missionFinish), lock_2(Mutex_imageBuf);
			Mat gray;
			cvtColor(imageBuf, gray, CV_BGR2GRAY);
			patternFind = findChessboardCorners(gray, board_size, corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE + CALIB_CB_FAST_CHECK);
			if (patternFind)
			{
				cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
				corners_Seq.push_back(corners);
				for (int i = 0; i < corners.size(); i++)
				{
					char buffer[4];
					sprintf(buffer, "%d", i);
					circle(imageBuf, corners[i], 5, Scalar(255,0, 0),2);
					putText(imageBuf, buffer, corners[i], 1, 2, Scalar(255, 0, 0), 2);
				}
				char  grayImgName[32];
				sprintf(grayImgName, "imageData/%05d.jpg", frameFound);
				imwrite(grayImgName, imageBuf);

				frameFound++;
			}
			printf("Need: %d, Found: %d, Time: %d  Size: %d\n", frameNeed, frameFound, timeCount++, corners.size());

			if (frameFound == frameNeed)
			{
				missionFinish = true;
				break;
			}

			lock_1.unlock();
			lock_2.unlock();
			this_thread::sleep_for(chrono::seconds(2));
		}
	}).detach();
}

void calibration()
{
	vector<Point3f> tempPointSet;
	for (int i = 0; i<board_size.height; i++)
	{
		for (int j = 0; j<board_size.width; j++)
		{
			/* 假设定标板放在世界坐标系中z=0的平面上 */
			Point3f tempPoint;
			tempPoint.x = i*100;
			tempPoint.y = j*100;
			tempPoint.z = 0;
			tempPointSet.push_back(tempPoint);
		}
	}
	for(int i = 0 ; i < usedImageForCalib; i++) object_Points.push_back(tempPointSet);

	Size image_size = Size(CAM_WIDTH, CAM_HEIGTH);
	vector<cv::Vec3d> rotation_vectors;    /* 每幅图像的旋转向量 */
	vector<cv::Vec3d> translation_vectors; /* 每幅图像的平移向量 */
	int flags = 0;
	flags |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
	flags |= cv::fisheye::CALIB_CHECK_COND;
	flags |= cv::fisheye::CALIB_FIX_SKEW;

	/*******由相机的定义使用两个不同的矫正方法*******/
	if(FISH_EYE)
		fisheye::calibrate(
			object_Points,
			corners_Seq,
			image_size,
			intrinsic_matrix,
			distortion_coeffs,
			rotation_vectors,
			translation_vectors,
			flags, cv::TermCriteria(3, 20, 1e-6));
	else
		cv::calibrateCamera(
			object_Points,
			corners_Seq,
			image_size,
			intrinsic_matrix,
			distortion_coeffs,
			rotation_vectors,
			translation_vectors, 0, cv::TermCriteria(3, 20, 1e-6));

	Mat R = Mat::eye(3, 3, CV_32F);
	Mat newIn;
	intrinsic_matrix.copyTo(newIn);
	newIn.at<double>(0, 2) = OUTPUT_WIDTH / 2 - WIDTH_ADJUST;
	newIn.at<double>(1, 2) = OUTPUT_HEIGTH / 2- HEIGHT_ADJUST;
	if (FISH_EYE)
		fisheye::initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, newIn, Size(OUTPUT_WIDTH, OUTPUT_HEIGTH), CV_32FC1, mapx, mapy);
	else
		initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, newIn, Size(OUTPUT_WIDTH, OUTPUT_HEIGTH), CV_32FC1, mapx, mapy);
}

int main()
{
	cout << "Calib cam ID is : ";
	cin >> camID;

	v.open(0);

	if (!v.isOpened()) return 0;
	else
	{
		v.set(CAP_PROP_FRAME_HEIGHT, CAM_HEIGTH);
		v.set(CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
	}

	if (!read())
	{
		cout << "******* Checking Board Start *******" << endl;
		chessCheck(usedImageForCalib);
		while (1)
		{
			unique_lock<mutex> lock_1(Mutex_missionFinish), lock_2(Mutex_patternFind);
			if (missionFinish)	break;
		}
		cout << "******** Checking Board End ********" << endl;

		cout << "********* calibration Start ********" << endl;
		calibration();
		cout << "********* calibration Start ********" << endl;
		save();
	}

	cout << "********** Display Start ***********" << endl;
	missionFinish = false;
	display(1);
	cout << "************ Display End ***********" << endl;
 
	waitKey(0);
}
 
