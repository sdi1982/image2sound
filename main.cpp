#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "math.h"
#include "audio_creator.h"

#include <iostream>
#include <string>

using namespace cv;
using namespace std;

Mat modifyImage(Mat image) {
  Mat res;
  // convert RGB to gray
  cvtColor(image, res, CV_BGR2GRAY);

  // resize image
  Size size(64, 64);
  resize(res, res, size);

  // normalize image
  equalizeHist(res, res);

  //res.convertTo(res, CV_32FC1);
  //normalize(res, res, 0, 1.0, NORM_MINMAX, CV_8UC1);
  // Mat img_pxl = res;
  // for(int y = 0; y < img_pxl.rows; ++y) {
  //   for(int x = 0; x < img_pxl.cols; ++x) {
  //     // get pixel
  //     float color = img_pxl.at<uchar>(x,y);
  //     color = color / 255.0;
      
  //     // set pixel
  //     img_pxl.at<uchar>(x,y) = color;
  //     //cout << "(" << x << ", " << y << ")" <<  color << endl;
  //   }
  // }

  return res;
}

void initializeAudio(double sample, double samp_freq) {
  Audio::Open();

  Audio::Close();
}

void img2freq(Mat input) {
  // sampling frequency
  double samp_freq = 8000;
  // create freq matrix and fill with a default value of 0
  double freq[64] = {0};
  
  // center frequency on 'A'
  // sequentially create higher and lower frequencies
  freq[32] = 440.0;
  for(int i = 33; i < 64; ++i) {
    freq[i] = freq[i-1] * pow(2.0, (1.0/12.0));
  }
  for(int i = 31; i > -1; --i) {
    freq[i] = freq[i+1] * pow(2.0, (-1.0/12.0));
  }

  // t = time == samples
  double t[500];
  for(int i = 0; i < 500; ++i) {
    t[i] = (i+1)/samp_freq;
  }

  // sound every column as a chord
  for(int col = 0; col < 64; ++col) {
    double signal[500] = {0};
    for(int row = 0; row < 64; ++row) {
      float value = input.at<uchar>(row,col);
      value = value / 255.0;
      int m = 64-row+1;
      double ss[500];
      for(int i = 0; i < 500; ++i) {
	ss[i] = sin(2.0*M_PI*freq[m]*t[i]);
	signal[i] = signal[i] + value * ss[i];
	//cout << signal[i] << endl;
      }
    }
    for(int i = 0; i < 500; ++i) {
      signal[i] = signal[i]/64;
      initializeAudio(signal[i], samp_freq);
      cout << signal[i] << endl;
    }
  }
  
  // uncomment to debug
  // for(int i = 0; i < 500; ++i) {
  //   cout << M_PI << endl;
  // }
}

// extracts frames to jpg from video file
void frameExtractor(CvCapture *capture) {
  int fps = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
  cout << "FPS: " << fps << endl;
  
  IplImage* frame = NULL;
  int frame_number = 0;
  char key = 0;

  while(key != 'q') {
    frame = cvQueryFrame(capture);
    if(!frame) {
      cout << "cvQueryFrame failed: no frame" << endl;
      break;
    }
    
    char filename[100];
    strcpy(filename, "pictures/frame_");
    
    char frame_id[30];
    sprintf(frame_id, "%d", frame_number);
    strcat(filename, frame_id);
    strcat(filename, ".jpg");
    
    cout << "Saving: " << filename << endl;
    
    if(!cvSaveImage(filename, frame)) {
      cout << "cvSaveImage failed" << endl;
      break;
    }

    frame_number++;
    
    key = cvWaitKey(1000/fps);
  }

  cvReleaseCapture(&capture);
}

int main(int argc, char** argv) {
  // reads video file as argument
  if(argc < 2) {
    cout << "Usage: ./program <filename>" << endl;
    return -1;
  }

  cout << "Filename: " << argv[1] << endl;
  
  CvCapture *capture = cvCaptureFromAVI(argv[1]);
  if(!capture) {
    cout << "File not found" << endl;
    return -1;
  }

  frameExtractor(capture);

  String path("pictures/*.jpg"); 
  vector<String> fn;
  
  glob(path, fn, true);
  for(size_t k = 0; k < fn.size(); ++k) {
    Mat im = imread(fn[k]);
    
    // exit if empty
    if(im.empty())
      continue;

    Mat res = modifyImage(im);

    img2freq(res);
  }

  
  return 0;
}
