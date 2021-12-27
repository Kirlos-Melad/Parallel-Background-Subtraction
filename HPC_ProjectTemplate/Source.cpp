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
int* inputImage(int* w, int* h, System::String^ imagePath);

// Save image to drive
void createImage(int* image, int width, int height, int index);

// Load multiple images
void getImages(int**& imgArr);

// Create an array of images in the heap
void initializeImageArray(int**& imgArr, int const sz);

// Free the heap from an array of images
void deleteImageArray(int**& imgArr, int const sz);

// Add Image Array
void addImageArray(void* inputBuffer, void* outputBuffer, int* len, MPI_Datatype* datatype);


// Get the mean of the pixels
void pixelMean(int* &img);

// Get the background image
int* backgroundExtraction(int** &imgArr);

int main()
{
	//Create array of images
	int** imgArr = new int* [IMAGE_NUMBER] {0};
	
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


void getImages(int** &imgArr) {
	System::String^ imagePath;
	string const path = "../Data/Input/";
	string imgName;
	string num;
	int internalLoop;

	int ImageWidth, ImageHeight, const totalDigits = 6;

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
		imgArr[i - 1] = inputImage(&ImageWidth, &ImageHeight, imagePath);
	}
}


void initializeImageArray(int** &imgArr, int const sz) {
	imgArr = new int* [sz];
	for (int i = 0; i < sz; i++)
		imgArr[i] = new int[IMAGE_SIZE] {0};
}


void deleteImageArray(int** &imgArr, int const sz) {
	for (int i = 0; i < sz; i++)
		delete[] imgArr[i];
	delete[] imgArr;
}


void addImageArray(void* inputBuffer, void* outputBuffer, int* len, MPI_Datatype* datatype) {
	// convert them into a proper datatype
	int** imgArr = (int**)inputBuffer;
	int* newImg = (int*)outputBuffer;
	int sum = 0;

	// int* len -> is the "count" argument passed to the reduction call.
	// or if called by user it's the array size

	for (int i = 0; i < IMAGE_SIZE; i++) {
		for (int j = 0; i < (*len); j++) {
			sum += imgArr[j][i];
		}
		newImg[i] += sum;
		sum = 0;
	}
}


void pixelMean(int* &img) {
	for (int i = 0; i < IMAGE_SIZE; i++)
		img[i] /= IMAGE_NUMBER;
}


int* backgroundExtraction(int** &imgArr) {
	int sz;
	int* retImg = new int[IMAGE_SIZE] {0};

	// Initialize the MPI env
	MPI_Init(NULL, NULL);

	// Get processes number
	int wSize;
	MPI_Comm_size(MPI_COMM_WORLD, &wSize);

	// Create Image Type in MPI
	MPI_Datatype MPI_IMAGE;
	MPI_Type_contiguous(IMAGE_SIZE, MPI_INT, &MPI_IMAGE);
	MPI_Type_commit(&MPI_IMAGE);

	// Create Add Image Operation in MPI
	MPI_Op MPI_ADD_IMAGE;
	MPI_Op_create((MPI_User_function*)addImageArray, 1, &MPI_ADD_IMAGE);

	// divide imgs on processors
	sz = IMAGE_NUMBER / wSize;
	int** buffer = nullptr;
	initializeImageArray(buffer, sz);

	cout << "Ready for Scatter .. Press any key\n";
	cin.get();

	MPI_Scatter(imgArr, sz, MPI_IMAGE, buffer, sz, MPI_IMAGE, 0, MPI_COMM_WORLD);

	cout << "Done Scatter .. Press any key\n";
	cin.get();

	// put the result from each processor
	int* sendImg = new int[IMAGE_SIZE] {0};
	addImageArray(buffer, sendImg, &sz, nullptr);

	// reduce the result at root processor = 0
	MPI_Reduce(sendImg, retImg, 1, MPI_IMAGE, MPI_ADD_IMAGE, 0, MPI_COMM_WORLD);

	int wRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &wRank);
	if (wRank == 0) {
		int* remainedImgs = new int[IMAGE_SIZE] {0};

		// Add remained images
		int start = wSize * (IMAGE_NUMBER / wSize);
		int imgNum = IMAGE_NUMBER;
		addImageArray(imgArr + start, remainedImgs, &imgNum, nullptr);

		// Add last 2 img
		imgNum = 1;
		addImageArray(remainedImgs, retImg, &imgNum, nullptr);

		// Divide the pixels on its number
		pixelMean(retImg);

		// Free memory
		delete[] remainedImgs;
	}

	// free memory
	deleteImageArray(buffer, sz);
	MPI_Type_free(&MPI_IMAGE);
	MPI_Op_free(&MPI_ADD_IMAGE);
	delete[] sendImg;

	// Finalize MPI env
	MPI_Finalize();

	return retImg;
}