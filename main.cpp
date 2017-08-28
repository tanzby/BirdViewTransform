#include  "birdView.hpp"

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
