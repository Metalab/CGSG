#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <unistd.h>

const char *threshwin = "threshold";
const char *outlinewin = "outline";

int main(int argc, char** argv) 
{
  int key;
  IplImage *img = cvLoadImage("../metalabumgebung.png", -1);
  IplImage *binarized, *contImg;
  CvMemStorage *storage;
  CvSeq *contours;

  binarized = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
  contImg = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 3);

  //just use the red channel for now
  cvSplit(img, binarized, NULL, NULL, NULL);

  //remove white
  cvThreshold(binarized, binarized, 250, 255, CV_THRESH_TOZERO_INV);

  
  cvNamedWindow(threshwin, CV_WINDOW_AUTOSIZE);
  cvShowImage(threshwin, binarized);

  //find outline
  storage = cvCreateMemStorage(0);
  cvFindContours(binarized, storage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL,
                  CV_CHAIN_APPROX_SIMPLE);

  cvDrawContours(contImg, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), 1, 1, CV_AA, cvPoint(0,0));

  cvNamedWindow(outlinewin, CV_WINDOW_AUTOSIZE);
  cvShowImage(outlinewin, contImg); 
  

  while (1) {
    key = cvWaitKey(10);
  
    switch (key) {
      case 'x':
        return 0;
    }
  }
  return 1;
}
