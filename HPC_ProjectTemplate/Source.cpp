#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
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


MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2) {
	return addr1 - addr2;
}

struct Image {
	int* const _img;
	unsigned int const _height, const _width;

	/*Image() {
		_img = nullptr;
		_width = _height = 0;
	}*/
	Image(int* img, unsigned h, unsigned w) : _img(img), _height(h), _width(w) {}
	~Image() {
		free((int*)_img); // https://stackoverflow.com/questions/2819535/unable-to-free-const-pointers-in-c
	}

	void operator=(Image other) {
		for (int i = 0; i < other._height; i++) {
			for (int j = 0; j < other._width; j++) {
				(*this)(i, j) = other(i, j);
			}
		}
	}

	int& operator()(unsigned row, unsigned col) {
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
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

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
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//" + marshal_as<System::String^>(rsltName) + index + ".png");
	cout << "result Image Saved " << rsltName << index << endl;
}

vector<Image>* readInput() {
	System::String^ imagePath;
	string const path = "..//Data//Input//";
	string imgName;

	vector<Image> imgVec;
	int* ImageArr, ImageWidth, ImageHeight, const imgNumber = 495, const totalDigits = 6;

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

	return &imgVec;
}

void saveOutput(vector<Image> &imgVec, string rsltName) {
	for (int i = 0; i < imgVec.size(); i++) {
		createImage(imgVec[i]._img, imgVec[i]._width, imgVec[i]._height, rsltName, i + 1);
	}
}

void muskEquation(vector<Image>& imgVec, int start, Image &bgImg, int const TH) {
	int result;
	for (int i = start; i < imgVec.size(); i++)
		for (int j = 0; j < imgVec[0]._height; j++)
			for (int k = 0; k < imgVec[0]._width; k++) {
				result = abs(bgImg(j, k) - imgVec[i](j, k));
				if (result > TH) {
					imgVec[i](j, k) = 0;
				}
				else {
					imgVec[i](j, k) = 255;
				}
			}
}

void create_MPI_Image_Type(MPI_Datatype &ImageType, Image& img) {
	// Length for each block
	int lengths[3] = { 1, 1,  img._height * img._width };

	// Displacement between blocks
	MPI_Aint displacements[3];
	MPI_Aint base_address;
	MPI_Get_address(&img, &base_address);
	MPI_Get_address(&img._width, &displacements[0]);
	MPI_Get_address(&img._height, &displacements[1]);
	MPI_Get_address(img._img, &displacements[2]);

	displacements[0] = MPI_Aint_diff(displacements[0], base_address);
	displacements[1] = MPI_Aint_diff(displacements[1], base_address);
	displacements[2] = MPI_Aint_diff(displacements[2], base_address);

	// Types for each block
	MPI_Datatype types[3] = { MPI_UINT32_T, MPI_UINT32_T, MPI_UINT32_T };

	// Commiting the new MPI type
	MPI_Type_create_struct(3, lengths, displacements, types, &ImageType);
	MPI_Type_commit(&ImageType);
}

void muskExtraction(vector<Image>& imgVec, Image &bgImg, int const TH, vector<Image> &muskVec) {
	vector<Image> buffer; // buffer for each processor, muskImg for root processor = 0

	// used for bgEquation fn
	int count = 0, start = 0;

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	int wSize;
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);

	// Get rank of this process
	int wRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);

	// Creating Image Type in MPI
	MPI_Datatype ImageType;
	create_MPI_Image_Type(ImageType, imgVec[0]);

	// divide imgs on processors
	count = imgVec.size() / wSize;
	buffer.resize(count, { new int[imgVec[0]._width * imgVec[0]._height], imgVec[0]._height, imgVec[0]._width });
	MPI_Scatter(&imgVec, count, ImageType, &buffer, count, ImageType, 0, MPI_COMM_WORLD);

	muskEquation(buffer, 0, bgImg, TH);

	MPI_Gather(&buffer, count, ImageType, &muskVec, muskVec.size(), ImageType, 0, MPI_COMM_WORLD);
	if (wRank == 0) {

		start = wSize * (imgVec.size() / wSize);
		muskEquation(muskVec, start, bgImg, TH);
	}

	// Finalize MPI env
	MPI_Finalize();
}

void bgEquation(vector<Image>& imgVec, Image& img, int& start, int& end) {
	unsigned int sum = 0;
	for (int i = 0; i < imgVec[0]._height; i++) {
		for (int j = 0; j < imgVec[0]._width; j++) {
			for (int k = start; k < end; k++) {
				sum += imgVec[k](i, j);
			}
			img(i, j) = sum;
			sum = 0;
		}
	}
}

void bgEquation(Image& img, int division) {
	for (int i = 0; i < img._height; i++) {
		for (int j = 0; j < img._width; j++) {
			img(i, j) /= division;
		}
	}
}

Image backgroundExtraction(vector<Image>& imgVec) {
	vector<Image> buffer, recvImg; // buffer for each processor, recvImg for root processor = 0

	int* arr = new int[imgVec[0]._width * imgVec[0]._height];
	Image sendImg(arr, imgVec[0]._height, imgVec[0]._width); // result from each processor

	// used for bgEquation fn
	int start = 0, end = 0;

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	int wSize;
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);
	// initialize vector with values
	recvImg.resize(wSize, { new int[imgVec[0]._width * imgVec[0]._height], imgVec[0]._height, imgVec[0]._width });

	// Get rank of this process
	int wRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);

	// Creating Image Type in MPI
	MPI_Datatype ImageType;
	create_MPI_Image_Type(ImageType, imgVec[0]);

	// divide imgs on processors
	end = imgVec.size() / wSize;
	buffer.resize(end, { new int[imgVec[0]._width * imgVec[0]._height], imgVec[0]._height, imgVec[0]._width });
	MPI_Scatter(&imgVec, end, ImageType, &buffer, end, ImageType, 0, MPI_COMM_WORLD);

	bgEquation(imgVec, sendImg, start, end);

	MPI_Gather(&sendImg, 1, ImageType, &recvImg, wSize, ImageType, 0, MPI_COMM_WORLD);
	if (wRank == 0) {
		vector<Image> finalStage(2, { new int[imgVec[0]._width * imgVec[0]._height], imgVec[0]._height, imgVec[0]._width });
		// Loop On Recived Images
		end = wSize;
		bgEquation(recvImg, finalStage[0], start, end);

		// Loop on remained images
		start = wSize * (imgVec.size() / wSize);
		end = imgVec.size();
		bgEquation(imgVec, finalStage[1], start, end);

		start = 0;
		end = 2;
		// loop on last 2 img
		bgEquation(finalStage, sendImg, start, end);

		bgEquation(sendImg, imgVec.size());
	}

	// Finalize MPI env
	MPI_Finalize();

	return sendImg;
}


int main()
{
	// Get Img Vector
	vector<Image> *imgVec = readInput();

	int start_s, stop_s, TotalTime = 0;
	start_s = clock();

	Image bgImg = backgroundExtraction((*imgVec));
	vector<Image> mskVec((*imgVec).size(), { new int[(*imgVec)[0]._width * (*imgVec)[0]._height], (*imgVec)[0]._height, (*imgVec)[0]._width });
	muskExtraction((*imgVec), bgImg, 50, mskVec);

	stop_s = clock();
	// Get Parrallelism time
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time: " << TotalTime << endl;

	// Save background and musk Vectors
	createImage(bgImg._img, bgImg._width, bgImg._height, "Background", 1);
	saveOutput(mskVec, "Musk");

	return 0;
}