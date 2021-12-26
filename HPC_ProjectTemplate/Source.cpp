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


// Image Global Variables
int const IMAGE_NUMBER = 40;
int const IMAGE_HEIGHT = 320;
int const IMAGE_WIDTH = 240;
int const IMAGE_SIZE = 76800;


// Load image to memory
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


// Save image to drive
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


// Copy pixels' values from an image to another
void copyImage(int dst[IMAGE_SIZE], int src[IMAGE_SIZE]) {
	for (int i = 0; i < IMAGE_SIZE; i++) {
		dst[i] = src[i];
	}
}


// Load multiple images
void getImages(int **imgArr) {
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

		// copy 'img' values to 'imgArr'
		copyImage(imgArr[i - 1], img);

		delete[] img;
	}
}


// create an array of images in the heap
void initializeImageArray(int **&imgArr, int const sz) {
	imgArr = new int* [sz];
	for (int i = 0; i < sz; i++)
		imgArr[i] = new int[IMAGE_SIZE];
}


// Free the heap from an array of images
void deleteImageArray(int** imgArr, int const sz) {
	for (int i = 0; i < sz; i++)
		delete[] imgArr[i];
	delete[] imgArr;
}


// overloaded function - get the sum of the pixels
void bgEquation(int **src, int start, int end, int* dst) {
	unsigned int sum = 0;
	for (int i = 0; i < IMAGE_SIZE; i++) {
		for (int j = start; i < end; j++) {
			sum += src[j][i];
		}
		dst[i] = sum;
		sum = 0;
	}
}


// overloaded function - get the mean of the pixels
void bgEquation(int* img) {
	for (int i = 0; i < IMAGE_SIZE; i++)
		img[i] /= IMAGE_NUMBER;
}


// return the background image
int* backgroundExtraction(int** imgArr) {
	// recvImg for root processor = 0
	int **recvImg = nullptr;
	int sz;
	int *retImg = new int[IMAGE_SIZE];

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	int wSize;
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);
	initializeImageArray(recvImg, wSize);
	
	// Creating Image Type in MPI
	MPI_Datatype MPI_IMAGE;
	MPI_Type_contiguous(IMAGE_SIZE, MPI_INT, &MPI_IMAGE);
	MPI_Type_commit(&MPI_IMAGE);

	// divide imgs on processors
	sz = IMAGE_NUMBER / wSize;
	int **buffer = nullptr;
	initializeImageArray(buffer, sz);
	MPI_Scatter(imgArr, sz, MPI_IMAGE, buffer, sz, MPI_IMAGE, 0, MPI_COMM_WORLD);

	int *sendImg = new int[IMAGE_SIZE];
	bgEquation(imgArr, 0, sz, sendImg);

	// we need to create a user defined operation and use reduce
	MPI_Gather(sendImg, 1, MPI_IMAGE, recvImg, 1, MPI_IMAGE, 0, MPI_COMM_WORLD);

	int wRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);
	if (wRank == 0) {
		int** finalStage = nullptr;
		initializeImageArray(finalStage, 2);

		// Loop On Recived Images
		bgEquation(recvImg, 0, wSize, finalStage[0]);

		// Loop on remained images
		int start = wSize * (IMAGE_NUMBER / wSize);
		bgEquation(imgArr, start, IMAGE_NUMBER, finalStage[1]);

		// loop on last 2 img
		bgEquation(finalStage, 0, 2, sendImg);

		// divide the pixels
		bgEquation(sendImg);

		deleteImageArray(finalStage, 2);
	}

	// free memory
	deleteImageArray(buffer, sz);
	deleteImageArray(recvImg, wSize);
	MPI_Type_free(&MPI_IMAGE);

	copyImage(retImg, sendImg);
	delete[] sendImg;

	// Finalize MPI env
	MPI_Finalize();

	return retImg;
}

int main()
{
	//Create array of images
	int** imgArr = nullptr;
	initializeImageArray(imgArr, IMAGE_NUMBER);
	
	// Fill the array with images
	getImages(imgArr);

	// Start Parrallel code
	int start_s = clock();
	
	// Get background image
	int *bgImg = backgroundExtraction(imgArr);

	// End Parrallel code
	int stop_s = clock();

	// Get Parrallelism time
	int TotalTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time: " << TotalTime << endl;

	// Save background image
	createImage(bgImg, IMAGE_WIDTH, IMAGE_HEIGHT, 0);

	//Free memory
	deleteImageArray(imgArr, IMAGE_NUMBER);
	delete[] bgImg;

	return 0;
}