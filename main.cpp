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
  res.convertTo(res, CV_32FC1);
  normalize(res, res, 0.0, 1.0, NORM_MINMAX);

  for(int y = 0; y < res.rows; ++y) {
    for(int x = 0; x < res.cols; ++x) {
      float pixel = res.at<float>(x,y);
      if(pixel > 0) 
	pixel = floor(pixel*16);
      if(pixel < 0)
	pixel = ceil(pixel*16);
      
      pixel = pixel / 16;

      res.at<float>(x,y) = pixel;
    }
  }

  return res;
}

void img2freq(Mat input) {
  Audio::Open();

  // create freq matrix and fill with a default value of 0
  float freq[64] = {0};
  
  // center frequency on 'A'
  // sequentially create higher and lower frequencies
  freq[31] = 440.0;
  for(int i = 32; i < 64; ++i) {
    freq[i] = freq[i-1] * pow(2.0, (1.0/12.0));
  }
  for(int i = 30; i > -1; --i) {
    freq[i] = freq[i+1] * pow(2.0, (-1.0/12.0));
  }
  
  const size_t n = 500;
  // click sound
  float click[n];
  // t = time == samples
  float t[n];
  for(int i = 0; i < n; ++i) {
    t[i] = (i+1)/Audio::GetFrequency();
    click[i] = sinf(2.0*float(M_PI)*50*t[i]);
  }

  // TODO: change signals to a 2d array and store all before playing
  // to reduce 'clunkiness' 
  // sound every column as a chord
  for(int col = 0; col < 64; ++col) {
    // 500 signals per column
    float signals[n] = {0};
    for(int row = 0; row < 64; ++row) {
      float value = input.at<float>(row,col);
      int m = 64-row;
      float ss[n];
      for(int i = 0; i < n; ++i) {
	ss[i] = sinf(2.0*float(M_PI)*freq[m]*t[i]);
	signals[i] = signals[i] + value * ss[i];
      }
    }
    for(int i = 0; i < n; ++i) {
      signals[i] = signals[i]/64;
    }
    Audio::Play(signals, n);
    Audio::WaitForSilence();
  }
  Audio::Play(click, n);
  Audio::WaitForSilence();
}

// extracts frames to jpg from video file
void frameExtractor(CvCapture *capture) {
  int fps = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
  
  IplImage* frame = NULL;
  int frame_number = 0;
  char key = 0;

  while(key != 'q') {
    frame = cvQueryFrame(capture);
    if(!frame)
      break;
    
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
  int frame_counter = 0;
  
  glob(path, fn, true);
  for(size_t k = 0; k < fn.size(); ++k) {
    Mat im = imread(fn[k]);
    
    // exit if empty
    if(im.empty())
      continue;

    Mat res = modifyImage(im);
    
    frame_counter++;
    cout << "Frame #" << frame_counter << endl;
    img2freq(res);
  }

  Audio::Close();

  // clear the pictures directory
  // TODO: BAD, FIX LATER IF POSSIBLE
  system("exec rm -r pictures/*");
  
  return 0;
}
