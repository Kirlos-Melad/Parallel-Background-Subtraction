#pragma once
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

class ImageHandler
{
private:
	int* _img, _height, _width;
public:
	ImageHandler();
	int* inputImage(System::String^ imagePath); //put the size of image in w & h
	void createImage(int* image, int width, int height, int index);
};

