#pragma once

#ifndef IMGPROC_H
#define IMGPROC_H

#pragma comment(lib, "cv.lib")
#pragma comment(lib, "highgui.lib")
#pragma comment(lib, "cxcore.lib")
#pragma comment(lib, "cvaux.lib")
//#pragma comment(lib, "cvblobslib.lib")
#pragma warning(disable:4244)
#pragma warning(disable:4996)

#include <cv.h>
#include "highgui.h"
#include "cvaux.h"
#include "imgprocaux.h"
//#include <blob.h>
//#include <blobresult.h>


//-------------------------------------------
// CONSTANS
//-------------------------------------------
#define		IP_MOTION_THRESHOLD		13
#define		MAX_TRACKER_NUM			2
#define		TRACK_WIND_WIDTH		40
#define		TRACK_WIND_HEIGHT		40
#define		MAX_COUNT				500
#define		SF						6
#define		TRACKER_THRESHOLD		2
#define		ALFA_STEP_COUNT			10
//-------------------------------------------
//-------------------------------------------

// definitions
// comment or uncomment it to enable or disable
//#define		MOTION_WITH_CANNY

typedef struct Tracker {
	int X, Y;
	int radius;
	int tracking;
	int state;
	//double motionLife;
} Tracker;

class ImageProcessor
{
private:
	// background
	CvBGStatModel*				bgModel;

	// frames
	long						currentFrameNumber;
	IplImage*					currentFrame;
	IplImage*					prevFrame;
	IplImage*					prevFrame2;
	IplImage*					meanImage;

	// haar
	CvHaarClassifierCascade*	cascade;
	CvHaarClassifierCascade*	cascadeH;
	CvSeq*						hands;
	CvSeq*						faces;
	
	// motion and optical flow
	int							motionFeaturesNumber;
	int							flag;
	int							feature_num;
	int							flock_num[1000];
	CvPoint2D32f				motionFeatures[1000],
								*features1,
								*features2,
								*swap_ftrs;
	IplImage					*pyr1, *pyr2, *swap_temp;

	// tracker
	int							trackerNumber;
	Tracker						trackers[MAX_TRACKER_NUM];
	bool						busy[400][400];

public:
	ImageProcessor(void);
	int GetNextFrame( IplImage* image );
	void ExtractBackground( IplImage* image );
	void CalcDensity( IplImage* image, CvMat* result );
	void FindMotionFeaturesCenter( CvPoint p1, float r1, CvPoint2D32f *p2, float *r2 );
	void FindDifference( IplImage* gray1, IplImage* gray2, IplImage* mask );
	void FindDifference3( IplImage* gray1, IplImage* gray2, IplImage* gray3, IplImage* mask, IplImage* mask2 );
	void FindMotionArtifacts( IplImage* gray1, IplImage* gray2, IplImage* gray3, CvPoint2D32f* allpoints, int* count );
	void DetectAndTrackHand( IplImage* image, int* X, int* Y, int* EN );
	void DetectFlow( IplImage* image1, IplImage* image2, IplImage* image3, IplImage* result );
	void DetectFace( IplImage* image );
	void DetectHands( IplImage* image );
	void GetSkinMask( IplImage* image, IplImage* mask );
	void Segment( IplImage* image, IplImage* result );

	void CreateTracker( int X, int Y, int radius, bool isface );
	void UpdateTracker( IplImage* gray, IplImage* mask );
	void RemoveTracker( int num );
	void DisplayTrackers( IplImage* image, IplImage* gray );
	void DrawFeature( CvPoint2D32f feature1, CvPoint2D32f feature2, IplImage* result, int param );
	void MoveTracker( int t_num, int X, int Y );

	int ExtractBlobDefects( IplImage* image );

public:
	~ImageProcessor(void);
};

#endif