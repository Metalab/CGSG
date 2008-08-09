#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <unistd.h>
#include <stdio.h>

const char *threshwin = "threshold";
const char *outlinewin = "outline";
const char *polywin = "polygon approximation";

int main(int argc, char** argv) 
{
  int key,i;
  IplImage *img = cvLoadImage("../metalabumgebung.png", 3);
  IplImage *binarized, *contImg;
  CvMemStorage *storage;
  CvSeq *contours, *polyContours;
  CvContour *contour;
  CvSeqReader reader;
  CvPoint *point;

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

  //approximate polygons
  polyContours = cvApproxPoly(contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 1, 1);
  
  cvSetZero(contImg);
  cvDrawContours(contImg, polyContours, CV_RGB(255,0,0), CV_RGB(0,255,0), 1, 1, CV_AA, cvPoint(0,0));
  cvNamedWindow(polywin, CV_WINDOW_AUTOSIZE);
  cvShowImage(polywin, contImg);

  //try to read outline of first poly contour and print to console
  printf("total = %d\n", polyContours->total);

  while(polyContours) 
  {
    printf("POLY:\n");
    for (i=0; i < polyContours->total; i++) 
    {
      point = (CvPoint*)cvGetSeqElem(polyContours, i);
      printf("   %4d, %4d\n", point->x, point->y);
    }
    printf("\n");
    polyContours = polyContours->h_next;
  }

/*
  cvStartReqdSeq(polyContours, &reader, 0);

  for (i=0; i < polyContours->total; i++) {
    CV_READ_SEQ_ELEM(contour, reader); 
  }
*/
  while (1) {
    key = cvWaitKey(10);
  
    switch (key) {
      case 'x':
        return 0;
    }
  }
  return 1;
}
