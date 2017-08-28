# BirdViewTransform
Put FOUR image (or video stream )from 4 different directions together, and form them into a bird view. 

## About

I have four imgs, such as(from up to down: **left**, **forward**, **right**, **backward**)

![Left](img/0.png)

![Forward](img/1.png)

![Right](img/2.png)

![Backward](img/3.png)

using this programe, we can get

![after transform](img/ok.png)

## parameters setting
Meanings of functions from this *Class BirhView* are shown as figure follow:

![param setting](img/ParamSetting.png)

Before using this API Class, be awared you have had known that accurated measure is need for better visual effect and making image be exact

## other

Before birdEye transform, Calibration of camera is needed! here is a [link](http://tanzby.cn/2017/08/01/%E5%9F%BA%E4%BA%8EopenCV%E7%9A%84%E7%9B%B8%E6%9C%BA%E6%A0%A1%E6%AD%A3%E7%A8%8B%E5%BA%8F/) for calibration. 
