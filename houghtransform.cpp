#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <opencv2/core.hpp>        //you may need to
#include <opencv2/highgui.hpp>   //adjust import locations
#include <opencv2/imgproc.hpp>    //depending on your machine setup
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

void convolute(Mat input_image, Mat output_image, Mat kernel);

void sobel(Mat input, Mat sobelX, Mat sobelY, Mat sobelMag, Mat sobelDir);

void thresholdX(Mat input, Mat output, int T);

void hough(Mat grad_mag, Mat grad_orient, int threshold, Mat org);

vector<Vec3f> houghCircleCalculation(Mat input, int minDist, int minRadius, int maxRadius);

// just for debugging
String type2str(int type);

int main() {
    cout << "Hello Dartboard Detector!" << endl;

    // input image
    String input_file_name = "dart1.jpg";
    Mat image = imread(input_file_name, 1);
    cout << "Loaded image '" << input_file_name << "' as input file." << endl;

    // Convert to gray scale
    Mat gray_image;
    cvtColor(image, gray_image, CV_BGR2GRAY);
    imwrite("gray_img.jpg", gray_image);
    cout << "Converted image to gray scale image." << endl;

    // Sobel filter
    cout << "Begin Sobel filter calculation..." << endl;
    Mat sobelX = Mat(gray_image.size(), CV_8U);//gray_image.clone();
    Mat sobelY = Mat(gray_image.size(), CV_8U);//gray_image.clone();
    Mat sobelMag = Mat(gray_image.size(), CV_8U);//gray_image.clone();
    Mat sobelDir = Mat(gray_image.size(), CV_8U);//gray_image.clone();
    sobel(gray_image, sobelX, sobelY, sobelMag, sobelDir);
    cout << "Finished Sobel calculation!" << endl;

    cout << "Begin thresholding sobelMag image..." << endl;
    Mat thresholdSobelMag = Mat(gray_image.size(), CV_8U);//gray_image.clone();
    thresholdX(sobelMag, thresholdSobelMag, 100);
    cout << "Finished thresholding sobelMag image!" << endl;

    cout << "Begin hough transformation..." << endl;
    hough(sobelMag,sobelDir, 100, image);
    cout << "Finished hough transformation!" << endl;

    sobelX.deallocate();
    sobelY.deallocate();
    sobelMag.deallocate();
    sobelDir.deallocate();
    thresholdSobelMag.deallocate();

    cout << "Done!" << endl;
    return 0;
}

void convolute(Mat input, Mat output, Mat kernel){
    // at the moment use the opencv convolution, maybe implement it later by yourself
     filter2D(input, output,-1, kernel);
}

// method for debugging
String type2str(int type) {
  String r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}


void sobel(Mat input, Mat sobelX, Mat sobelY, Mat sobelMag, Mat sobelDir){

    // deriative in x direction
    Mat kernelX(3, 3, CV_32F);
    kernelX.at<float>(0,0) = 1.0f;
    kernelX.at<float>(0,1) = 0.0f;
    kernelX.at<float>(0,2) = -1.0f;
    kernelX.at<float>(1,0) = 2.0f;
    kernelX.at<float>(1,1) = 0.0f;
    kernelX.at<float>(1,2) = -2.0f;
    kernelX.at<float>(2,0) = 1.0f;
    kernelX.at<float>(2,1) = 0.0f;
    kernelX.at<float>(2,2) = -1.0f;

    convolute(input,sobelX,kernelX);

    // and in y direction
    Mat kernelY(3, 3, CV_32F);
    kernelY.at<float>(0,0) = 1.0f;
    kernelY.at<float>(0,1) = 2.0f;
    kernelY.at<float>(0,2) = 1.0f;
    kernelY.at<float>(1,0) = 0.0f;
    kernelY.at<float>(1,1) = 0.0f;
    kernelY.at<float>(1,2) = 0.0f;
    kernelY.at<float>(2,0) = -1.0f;
    kernelY.at<float>(2,1) = -2.0f;
    kernelY.at<float>(2,2) = -1.0f;

    convolute(input,sobelY,kernelY);

    //imwrite("sobelY2.jpg", sobelY);

    /*sobelY = Mat(341,441, CV_8U, Scalar(255));
    for(int r = 0; r < sobelY.rows; r++){
        for(int c = 0; c < sobelY.cols; c++){
            sobelY.at<short>(r,c) = 100;
          }
        }*/
    // calculate the magnitude of the gradient
    // here the approximation of |G| = |G_x| + |G_y| is used instead of |G|=(G_x^2 + G_y^2)^0.5
  //  printf("cols: %d, rows: %d \n", sobelMag.cols, sobelMag.rows);

    for(int y = 0; y < input.rows; y++){
        for(int x = 0; x < input.cols; x++){
            //printf("gx: %d \n", sobelX.at<int>(x,y));
            float gx = abs(sobelX.at<float>(y,x));// >= 0 ? sobelX.at<int>(x,y) : 1-sobelX.at<int>(x,y);
            float gy = abs(sobelY.at<float>(y,x));// >= 0 ? sobelY.at<int>(x,y) : 1-sobelY.at<int>(x,y);
            //float gy = 0;
            /*if(gx >= 255){
              gx = 255;
            }

            if(gy >= 255){
              gy = 255;
            }
            if(gx < 0){
              gx = 0;
            }
            if(gy < 0){
              gy = 0;
            }

            */
            float g = (gx + gy);
            //printf("x: %d, y: %d \n", x, y);
            /*if(g > 255 || g < 0){
              printf("Not supported value: %f \n", g);
              printf("gx: %f, gy: %f\n", gx,gy);
            }*/
            //sobelMag.at<int>(y,x) = (float) g;
            
            sobelMag.at<float>(y,x) = (float) g;

            // TODO: check if overflow is possible here...
        }
    }
    cout << "Type of sobelX: " << type2str(sobelX.type()) << endl;
    cout << "Type of sobelY: " << type2str(sobelY.type()) << endl;
    cout << "Type of sobelMag: " << type2str(sobelMag.type()) << endl;

    imwrite("sobelGradientMagnitude.jpg", sobelMag);
    // calculate the direction of the gradient
    // the orientation O = arctan(G_y / G_x)
    for(int y = 0; y < input.rows; y++){
        for(int x = 0; x < input.cols; x++){
            float gx = sobelX.at<float>(y,x);
            float gy = sobelY.at<float>(y,x);
            float orient = (float) atan(gy / gx);
            /*if(orient > 255){
              orient = 255;
            }
            if(orient < 0){
              orient = 0;
            }*/
            
            sobelDir.at<float>(y,x) = orient;// * 180/CV_PI;
            
             // TODO: maybe use this  * 180/CV_PI;
             // TODO: check if overflow or underflow is possible here...
        }
    }

    // save all images
    imwrite("sobelX.jpg", sobelX);
    imwrite("sobelY.jpg", sobelY);
    imwrite("sobelGradientDirection.jpg", sobelDir);
}

void thresholdX(Mat input, Mat output, int T){
    // simple threshold calculation
    for(int y = 0; y < input.rows; y++){
        for(int x = 0; x < input.cols; x++){
            uchar pixel = input.at<uchar>(y,x);
            if(pixel >= T){
                output.at<uchar>(y,x) = 255;
            }else{
                output.at<uchar>(y,x) = 0;
            }
        }
    }

    // save the threshold image
    imwrite("threshold.jpg", output);
}


void hough(Mat grad_mag, Mat grad_orient, int threshold, Mat org){

    Mat src = org.clone();//Mat(341,441, CV_8U, Scalar(255));

    vector<Vec3f> circles;

    /// Apply the Hough Transform to find the circles
    circles = houghCircleCalculation( grad_mag, grad_mag.rows/8, 0, 0 );
    /// Draw the circles detected
    cout << circles.size() << endl;
    for( size_t i = 0; i < circles.size(); i++ )
    {
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);
       
        cout << "Radius is: " << radius << endl;
        // circle center
        circle( src, center, 3, Scalar(0,255,0), -1, 8, 0 );
        // circle outline
        circle( src, center, radius, Scalar(0,0,255), 3, 8, 0 );
    }
    /// Show your results
    namedWindow( "Hough Circle Transform", CV_WINDOW_AUTOSIZE );
    imshow( "Hough Circle Transform", src );
    imwrite("hough.jpg", src);
    waitKey(0);
}

vector<Vec3f> houghCircleCalculation(Mat input, int minDist, int minRadius, int maxRadius){
    // reimplement this
    // HoughCircles(input, output,CV_HOUGH_GRADIENT, 1, minDist, 200, 100, minRadius, maxRadius);
    Mat src = Mat(input.size(), CV_8U); //input.clone();

   // imshow( "Hough Circle Transform ZZZ", input );
    //waitKey(0);
    vector<Vec3f> output;
    cout << "Checkpoint 1" << endl;
    // init houghspace H with 0 everwhere
    int H[input.cols][input.rows][1];
    for(int i = 0; i < input.cols; i++){
        for(int j = 0; j < input.rows; j++){
            for(int k = 0; k < 1; k++){
                H[i][j][k] = 0;
            }
        }
    }

    cout << "Checkpoint 2" << endl;

    // calculate houghspace
    int t1 = 200;
    int r = 53;
    int counter = 0;
    for(int y = 0; y < input.rows-4; y++){
        for(int x = 0; x < input.cols-4; x++){  
            uchar pixel = input.at<uchar>(y,x);
            if(pixel >= t1){
                //circle(src, Point(x,y), r, Scalar(0,255,0),1,8,0);
                //counter ++;
                //cout << "abc " << counter << endl;        
                for(int theta = 0; theta < 350; theta=theta+4){
                //int theta = 0;
                    // calculate the polar coordinates for the center
                    
                    int a = x - r * cos(theta * CV_PI / 180);
                    if(a < 0 || a >= input.cols){
                        continue;
                    }
                    int b = y - r * sin(theta * CV_PI / 180);
                    if(b < 0 || b >= input.rows){
                        continue;
                    }
                    cout << "x: " << x << endl;
                    cout << "y: " << y << endl;

                   // cout << "input.cols: " << input.cols << endl;
                   // cout << "input.rows: " << input.rows << endl;

                    // increase voting
                    H[a][b][r] += 1;
                    cout << "Increment!" << endl;
                }
            }
        }
    }

    cout << "Checkpoint 3" << endl;

   // imshow( "Abc Transform", src );
   // waitKey(0);

    // threshold
    int t = 20;
    for(int i = 0; i < input.cols; i++){
        for(int j = 0; j < input.rows; j++){
            for(int k = 53; k <= 53; k++){
                if (H[i][j][k] > t){
                    // circle detected
                    cout << "Circle detected!" << endl;
                    output.push_back(Vec3f(i,j,k));
                }
            }
        }
    }

    cout << "Checkpoint 4" << endl;
   // cout << output << endl;
    cout << output.size() << endl;
    return output;
}

