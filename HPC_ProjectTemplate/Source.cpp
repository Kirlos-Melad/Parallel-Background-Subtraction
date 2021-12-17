#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include <mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

struct Image {
	int* const _img;
	unsigned int const _height, const _width;

	Image(int* img, unsigned h, unsigned w) : _img(img), _height(h), _width(w){}
	~Image() {
		free((int*)_img); // https://stackoverflow.com/questions/2819535/unable-to-free-const-pointers-in-c
	}

	int operator()(unsigned row, unsigned col) {
		if (row >= _height || col >= _width)
			throw "image out of bounds";
		return _img[_width * row + col];
	}
};

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, string rsltName, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//" + marshal_as<System::String^>(rsltName) + index + ".png");
	cout << "result Image Saved " << rsltName << index << endl;
}

vector<Image> readInput() {
	System::String^ imagePath;
	string const path = "..//Data//Input//";
	string imgName;

	vector<Image> imgVec;
	int *ImageArr, ImageWidth, ImageHeight, const imgNumber = 495, const totalDigits = 6;

	for (int i = 1; i <= imgNumber; i++) {
		// Get Img Name
		imgName = "in";
		string num = to_string(i);

		int const internalLoop = totalDigits - num.size();
		for (int j = 0; j < internalLoop; j++) {
			imgName += '0';
		}
		imgName += num;

		// Get Img
		imagePath = marshal_as<System::String^>(path + imgName);
		ImageArr = inputImage(&ImageWidth, &ImageHeight, imagePath);

		// Append Img to the vector
		imgVec.emplace_back(ImageArr, ImageHeight, ImageWidth);
	}

	return imgVec;
}

void saveOutput(vector<Image> imgVec, string rsltName) {
	for (int i = 0; i < imgVec.size(); i++) {
		createImage(imgVec[i]._img, imgVec[i]._width, imgVec[i]._height, rsltName,  i + 1);
	}
}

vector<Image> muskExtraction(vector<Image> bgVec, int const TH) {
	vector<Image> mskVec;

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	int wSize;
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);

	// Get rank of this process
	int wRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);

	// Finalize MPI env
	MPI_Finalize();

	return mskVec;
}

vector<Image> backgroundExtraction(vector<Image> imgVec) {
	vector<Image> bgVec;

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	int wSize;
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);

	// Get rank of this process
	int wRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);

	// Finalize MPI env
	MPI_Finalize();

	return bgVec;
}


int main(int argc, char **argv)
{
	// Get Img Vector
	vector<Image> imgVec = readInput();

	int start_s, stop_s, TotalTime = 0;
	start_s = clock();

	vector<Image> bgVec = backgroundExtraction(imgVec);
	vector<Image> mskVec = muskExtraction(bgVec, 20);
	
	stop_s = clock();
	// Get Parrallelism time
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time: " << TotalTime << endl;

	// Save background and musk Vectors
	saveOutput(bgVec, "Background");
	saveOutput(mskVec, "Musk");

	return 0;

}