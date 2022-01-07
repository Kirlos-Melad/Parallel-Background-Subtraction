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

// Image Global Macros
#define IMAGE_NUMBER 495
#define IMAGE_HEIGHT 240
#define IMAGE_WIDTH 320
#define IMAGE_SIZE 76800

// MPI Global Variables
int wSize, wRank;
const int rootProcessor = 0;

// Equation for comparing the original image's pixels
void muskEquation(int* src, int start, int end, int* dst);

// to extract the musk using the background and the original img
int* muskExtraction(int* img);

// Load image to memory
int* inputImage(int* w, int* h, System::String^ imagePath);

// Save image to drive
void createImage(int* image, int width, int height, int index);

// Load multiple images
void getImages(int** imgArr);

// Copy Image from source to destination
void copyImage(int* dst, int* src);

// Create an array of images in the heap
void initializeImageArray(int**& imgArr, int const sz);

// Free the heap from an array of images - the array must be initialized using initializeImageArray()
void deleteImageArray(int** imgArr, int const sz);

// Add Image Array
void addImageArray(int** inputBuffer, int* outputBuffer, int len);

// Get the mean of the pixels
void pixelMean(int* img);

// Get the background image
int* backgroundExtraction(int** imgArr);

int main()
{
	// According to this answer https://stackoverflow.com/questions/2156714/how-to-speed-up-this-problem-by-mpi/2290951#2290951
	// All the code works in parallel not only MPI_Init() & MPI_Finalize() part

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);
	// Get processor rank
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);

	//Create array of images
	int** imgArr = nullptr;
	initializeImageArray(imgArr, IMAGE_NUMBER);

	// Sequential part
	if (wRank == rootProcessor) {
		// Fill the array with images - only rootProcessor needs the values
		getImages(imgArr);
	}

	// Start Parrallel code
	int start_s = clock();

	// Get background image
	int* bgImg = backgroundExtraction(imgArr);

	// Get musk image
	int* musk = muskExtraction(bgImg);

	// End Parrallel code
	int stop_s = clock();

	// Get Parrallelism time
	int TotalTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;

	// Sequential part
	if (wRank == rootProcessor) {
		// print Rank rootProcessor time only
		cout << "time: " << TotalTime << endl;

		// Save background image
		createImage(bgImg, IMAGE_WIDTH, IMAGE_HEIGHT, 1);

		// Save musk image
		createImage(musk, IMAGE_WIDTH, IMAGE_HEIGHT, 2);

		//Free memory - only rootProcessor will return a value
		delete[] bgImg;
		delete[] musk;
	}

	// Free memory
	deleteImageArray(imgArr, IMAGE_NUMBER);

	// Finalize MPI env
	MPI_Finalize();

	return 0;
}


int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//********************Read Image and save it to local arrayss********	
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


void createImage(int* image, int width, int height, int index)
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
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}


void getImages(int** imgArr) {
	System::String^ imagePath;
	string const path = "../Data/Input/";
	string imgName;
	string num;
	int internalLoop;

	int* img, ImageWidth, ImageHeight, const totalDigits = 6;

	for (int i = 1; i <= IMAGE_NUMBER; i++) {
		// Get Img Name
		imgName = "in";
		num = to_string(i);

		internalLoop = totalDigits - num.size();
		for (int j = 0; j < internalLoop; j++) {
			imgName += '0';
		}
		imgName += num;
		imgName += ".jpg";

		// Get Img
		imagePath = marshal_as<System::String^>(path + imgName);
		img = inputImage(&ImageWidth, &ImageHeight, imagePath);

		copyImage(imgArr[i - 1], img);

		delete[] img;
	}
}


void copyImage(int* dst, int* src) {
	for (int i = 0; i < IMAGE_SIZE; i++)
		dst[i] = src[i];
}


void initializeImageArray(int**& imgArr, int const sz) {
	// Initialize contiguous dynamic array
	int* tmp = new int[sz * IMAGE_SIZE]{ 0 };

	// Convert it to 2D array
	imgArr = new int* [sz];
	for (int i = 0; i < sz; i++)
		imgArr[i] = tmp + (i * IMAGE_SIZE);
}


void deleteImageArray(int** imgArr, int const sz) {
	delete[] * imgArr;
	delete[] imgArr;
}


void addImageArray(int** inputBuffer, int* outputBuffer, int len) {
	if (len == 0)
		return;

	int sum = 0;
	for (int i = 0; i < IMAGE_SIZE; i++) {
		for (int j = 0; j < len; j++) {
			sum += inputBuffer[j][i];
		}
		outputBuffer[i] += sum;
		sum = 0;
	}
}


void pixelMean(int* img) {
	for (int i = 0; i < IMAGE_SIZE; i++)
		img[i] /= IMAGE_NUMBER;
}


int* backgroundExtraction(int** imgArr) {
	// Final Image
	int* retImg = new int[IMAGE_SIZE] {0};

	// divide imgs on processors
	int sz = IMAGE_NUMBER / wSize;

	// Create a buffer for each processor
	int** buffer = nullptr;
	initializeImageArray(buffer, sz);

	// Scatter the data on the processors
	MPI_Scatter(*imgArr, sz * IMAGE_SIZE, MPI_INT, *buffer, sz * IMAGE_SIZE, MPI_INT, rootProcessor, MPI_COMM_WORLD);

	// put the result from each processor
	int* sendImg = new int[IMAGE_SIZE] {0};
	// Add the pixels
	addImageArray(buffer, sendImg, sz);

	// reduce the result at root processor = 0
	MPI_Reduce(sendImg, retImg, IMAGE_SIZE, MPI_INT, MPI_SUM, rootProcessor, MPI_COMM_WORLD);

	// Sequential part
	if (wRank == rootProcessor) {
		int* remainedImgs = new int[IMAGE_SIZE] {0};

		// Add remained images
		int start = wSize * (IMAGE_NUMBER / wSize);
		int imgNum = IMAGE_NUMBER - start;
		addImageArray(imgArr + start, remainedImgs, imgNum);

		// Add last 2 img
		addImageArray(&remainedImgs, retImg, 1);

		// Divide the pixels on its number
		pixelMean(retImg);

		// Free memory
		delete[] remainedImgs;
	}

	// free memory
	deleteImageArray(buffer, sz);
	delete[] sendImg;
	if (wRank != rootProcessor) {
		delete[] retImg;
		retImg = nullptr;
	}

	return retImg;
}


int* muskExtraction(int* const Img) {
	// recvImg for root processor = 0
	int sz;

	// divide img pixels on processors
	sz = IMAGE_SIZE / wSize;
	System::String^ path;
	int* buffer = new int[sz];
	int* buffer2 = new int[sz];
	path = marshal_as<System::String^>("../Data/Input/in000256.jpg");
	int ImageW = 0, ImageH = 0;
	int* sendImg = inputImage(&ImageW, &ImageH, path);

	MPI_Scatter(sendImg, sz, MPI_INT, buffer2, sz, MPI_INT, rootProcessor, MPI_COMM_WORLD);

	MPI_Scatter(Img, sz, MPI_INT, buffer, sz, MPI_INT, rootProcessor, MPI_COMM_WORLD);

	muskEquation(buffer, 0, sz, buffer2);

	MPI_Gather(buffer2, sz, MPI_INT, sendImg, sz, MPI_INT, rootProcessor, MPI_COMM_WORLD);

	if (wRank == rootProcessor) {

		muskEquation(Img, sz * wRank, IMAGE_SIZE, sendImg);
	}


	// Free memory
	delete[] buffer;
	delete[] buffer2;
	if (wRank != rootProcessor) {
		delete[] sendImg;
		sendImg = nullptr;
	}

	return sendImg;
}

void muskEquation(int* src, int start, int end, int* dst) {
	int TH = 60;
	for (start; start < end; start++) {
		// new img = bgimg - original img
		// if > TH then change to the result
		if (abs(dst[start] - src[start]) > TH) {
			dst[start] = dst[start];
		}
		// smaller thant TH then black
		else {
			dst[start] = 0;
		}
	}
}