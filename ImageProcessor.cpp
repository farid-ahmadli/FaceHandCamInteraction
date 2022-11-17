#include "StdAfx.h"
#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(void)
{
	currentFrameNumber = 0;
	currentFrame = 0;
	prevFrame = 0;
	prevFrame2 = 0;
	cascade = 0;
	cascadeH = 0;
	trackerNumber = 0;
	bgModel = 0;
	meanImage = 0;
	hands = 0;
	faces = 0;
	feature_num = 0;
	flag = 0;
	memset( flock_num, -1, sizeof(flock_num) );

	features1 = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(CvPoint2D32f));
	features2 = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(CvPoint2D32f));

	memset(busy, 0, sizeof(busy) );

	pyr1 = 0;
	pyr2 = 0;
}

ImageProcessor::~ImageProcessor(void)
{
	if ( currentFrame ) cvReleaseImage( &currentFrame );
	if ( prevFrame ) cvReleaseImage( &prevFrame );
}

int ImageProcessor::GetNextFrame( IplImage* image )
{
	cvSmooth( image, image, 2 );
#ifdef EQUALIZE
	IplImage* ch1 = cvCreateImage( cvGetSize(image), 8, 1 );
	IplImage* ch2 = cvCreateImage( cvGetSize(image), 8, 1 );
	IplImage* ch3 = cvCreateImage( cvGetSize(image), 8, 1 );
	cvSplit( image, ch1, ch2, ch3, 0 );
	cvEqualizeHist( ch1, ch1 );
	cvEqualizeHist( ch2, ch2 );
	cvEqualizeHist( ch3, ch3 );
	cvMerge( ch1, ch2, ch3, 0, image );
	cvReleaseImage( &ch1 );
	cvReleaseImage( &ch2 );
	cvReleaseImage( &ch3 );
#endif

	//ExtractBackground( image );
	if ( currentFrame == 0 ) {
		currentFrame = cvCloneImage( image );
	}
	else {
		if ( prevFrame == 0 )
			prevFrame = cvCloneImage( currentFrame );
		else {
			if ( prevFrame2 == 0 )
				prevFrame2 = cvCloneImage( prevFrame );
			else
				cvCopy( prevFrame, prevFrame2 );
			cvCopy( currentFrame, prevFrame );
		}
		cvCopy( image, currentFrame );
	}
	return ++currentFrameNumber;
}

/*
*	Extracting the background
*/
void ImageProcessor::ExtractBackground( IplImage* image )
{
	if ( bgModel == 0 )
		bgModel = cvCreateGaussianBGModel ( image );
	else {
		//cvSetImageCOI( image, 3 );
		//cvRefineForegroundMaskBySegm
		cvUpdateBGStatModel( image, bgModel );
	}
}


/*
*	Calculates density of points
*/
void ImageProcessor::CalcDensity( IplImage* image, CvMat* result )
{
	if ( !result ) return;

	int w = image->width;
	int h = image->height;

	float sum[400][400];

	// initializing sum matrix
	int index = 0;
	for ( int i=0; i<image->height; i++ ) {
		for ( int j=0; j<image->width; j++ ) {
			sum[i][j] = image->imageData[index]>0;
			index++;
		}
	}

	// calculating sum on rows
	for ( int r=0; r<h; r++ )
		for ( int c=1; c<w; c++ )
			sum[r][c] += sum[r][c-1];

	// calculating sum on columns
	for ( int c=0; c<w; c++ )
		for ( int r=1; r<h; r++ )
			sum[r][c] += sum[r-1][c];

	int wSize = 20;
	// calculate density
	index = 0;
	for ( int r=0; r<h; r++ ) {
		for ( int c=0; c<w; c++ ) {			
			result->data.fl[index] = sum[ min(h-1,r+wSize) ][ min(w-1,c+wSize) ]
			- sum[ max(0,r-wSize-1) ][ min(w-1,c+wSize) ]
			- sum[ min(h-1,r+wSize) ][ max(0,c-wSize-1) ]
			+ sum[ max(0,r-wSize-1) ][ max(0,c-wSize-1) ];
			//if ( image->imageData[index] == 0 )
			image->imageData[index] = (int)255*(result->data.fl[index]>0);
			index++;

		}
	}
}

void ImageProcessor::FindMotionFeaturesCenter( CvPoint p1, float r1, CvPoint2D32f *p2, float *r2 )
{
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* seq = cvCreateSeq( CV_SEQ_KIND_GENERIC|CV_32SC2, sizeof(CvContour), sizeof(CvPoint), storage );

	for ( int i=0; i<motionFeaturesNumber; i++ ) {
		if ( dist(motionFeatures[i].x,motionFeatures[i].y,p1.x,p1.y) <= r1 ) {
			cvSeqPush( seq, &cvPoint((int)motionFeatures[i].x,(int)motionFeatures[i].y) );
		}
	}

	if ( seq->total > 0 ) {
		cvMinEnclosingCircle( seq, p2, r2 );
	}
	else {
		*p2 = cvPoint2D32f(p1.x,p1.y);
		*r2 = r1;
	}
	cvReleaseMemStorage( &storage );
}

void ImageProcessor::FindDifference( IplImage* gray1, IplImage* gray2, IplImage* mask )
{
	int index = 0;
	for ( int i=0; i<gray1->height; i++ ) {
		for ( int j=0; j<gray1->width; j++ ) {
			unsigned char value1 = gray1->imageData[index];
			unsigned char value2 = gray2->imageData[index];
			unsigned char value = abs(value1-value2);

			mask->imageData[index] = 255*( value > IP_MOTION_THRESHOLD );
			index++;
		}
	}
}

/*
*	Find difference between three images
*/
void ImageProcessor::FindDifference3( IplImage* gray1, IplImage* gray2, IplImage* gray3, IplImage* mask, IplImage* mask2 )
{
	if ( meanImage == 0 ) {
		meanImage = cvCreateImage( cvGetSize(gray1), 8, 1 );
		cvSet( meanImage, cvScalarAll(255) );
	}
	int index = 0;
	for ( int i=0; i<gray1->height; i++ ) {
		for ( int j=0; j<gray1->width; j++ ) {
			unsigned char value1 = gray1->imageData[index];
			unsigned char value2 = gray2->imageData[index];
			unsigned char value3 = gray3->imageData[index];
			unsigned char diff1 = abs(value1-value2);
			unsigned char diff2 = abs(value2-value3);
			//unsigned char result = (15*meanImage->imageData[index]+(int)value2);
			//meanImage->imageData[index] = result;

			mask->imageData[index] = 255*( diff1>IP_MOTION_THRESHOLD )&&( diff2>IP_MOTION_THRESHOLD );
			index++;
		}
	}
}

/*
*	Finds contour points from the motion path derived from difference of consecutive frames
*/
void ImageProcessor::FindMotionArtifacts( IplImage* gray1, IplImage* gray2, IplImage* gray3, CvPoint2D32f* allpoints, int* count )
{
	IplImage *tmp = cvCreateImage( cvGetSize(gray1), gray1->depth, gray1->nChannels );
	IplImage *tmp2 = cvCreateImage( cvGetSize(gray1), gray1->depth, gray1->nChannels );

	//FindDifference3( gray1, gray2, tmp );
	//FindDifference( gray2, gray3, tmp2 );
	FindDifference3( gray1, gray2, gray3, gray1, tmp );

	//cvAnd( tmp, tmp2, gray1 );
	cvDilate( gray1, gray1, 0, 2 );
	//cvErode( gray1, gray1, 0, 2 );

	//cvSmooth( gray1, gray1, CV_MEDIAN );


	//cvAnd( tmp, bgModel->foreground, tmp );

	//cvReleaseImage( &tmp );


#ifdef MOTION_WITH_CANNY
	cvCanny( gray2, tmp2, 30, 80 );
	cvAnd( tmp, tmp2, tmp );
#endif

	//return;

	int n = 0;

#ifdef FIND_CONTOURS
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contours = 0;

	// find contours
	cvFindContours( gray1, storage, &contours, sizeof(CvContour),
		CV_RETR_EXTERNAL , CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

	// if any contours found
	if ( contours ) {
		contours = cvApproxPoly( contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 4 , 1 ); // approximate

		CvSeq* cnt = contours;
		// go through all contours
		for ( ; cnt; cnt = cnt->h_next ) {
			CvPoint* pnts = (CvPoint*)malloc( cnt->total*sizeof(CvPoint) );
			cvCvtSeqToArray( cnt, pnts, CV_WHOLE_SEQ );
			// save points to the sequence
			if ( cnt->total > 2 ) {
				for ( int i=0; i<cnt->total; i++ ) {
					allpoints[n] = cvPoint2D32f( pnts[i].x, pnts[i].y );
					n++;
					if ( n > *count ) break;
				}
			}
			free( pnts );
			if ( n > *count ) break;
		}
		//		cvZero( tmp );
		cvDrawContours( tmp, contours, CV_RGB(255,255,255), CV_RGB(255,255,255), 3, 1, CV_AA, cvPoint(0,0) );

	}

	*count = n;
	cvReleaseMemStorage( &storage );
#else
	*count = 0;
#endif

	// free memory
	cvReleaseImage( &tmp );
	cvReleaseImage( &tmp2 );
}

/*
*	Function detects and tracks the hand position 
*/
void ImageProcessor::DetectAndTrackHand( IplImage* image, int* X, int* Y, int* EN )
{

	if ( GetNextFrame( image ) > 5 ) {
		//DetectFace( image );
		DetectFlow( prevFrame2, prevFrame, currentFrame, image );
	}

	if ( trackerNumber > 0 ) {
		*X = trackers[1].X;
		*Y = trackers[1].Y;
		*EN = trackers[1].state;

	}
	else {
		*X = image->width/2;
		*Y = image->height/2;
	}
}

/*
*	Function detects flow between two images
*/
void ImageProcessor::DetectFlow( IplImage* image1, IplImage* image2, IplImage* image3, IplImage* result )
{
	char found[1000];
	double max_dens = 0;

	// initialization
	if ( !flag ) {
		pyr1 = cvCreateImage( cvGetSize(image1), 8, 1 );
		pyr2 = cvCreateImage( cvGetSize(image2), 8, 1 );
		feature_num = 0;
	}

	// initalizations
	IplImage *eig = cvCreateImage( cvGetSize(image1), 32, 1 );
	IplImage *temp = cvCreateImage( cvGetSize(image1), 32, 1 );
	IplImage *gray1 = cvCreateImage( cvGetSize(image1), 8, 1 );
	IplImage *gray2 = cvCreateImage( cvGetSize(image2), 8, 1 );
	IplImage *gray3 = cvCreateImage( cvGetSize(image3), 8, 1 );
	IplImage* tmp_gray = cvCreateImage( cvGetSize(gray1), 8, 1 );
	CvMat* density = cvCreateMat( gray1->height, gray1->width, CV_32FC1 );

	cvConvertImage( image1, gray1 );
	cvConvertImage( image2, gray2 );
	cvConvertImage( image3, gray3 );

	if ( feature_num == 0 ) {
		feature_num = MAX_COUNT;
		cvGoodFeaturesToTrack( gray2, eig, temp, features1, &feature_num, .01, 5, NULL, 3, 1 );
		cvFindCornerSubPix( gray2, features1, feature_num, cvSize(10,10), cvSize(-1,-1),
			cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03) );
	}

	if ( feature_num > 0 ) {
		cvZero( tmp_gray );
		cvCalcOpticalFlowPyrLK( gray2, gray3, pyr1, pyr2, features1, features2, feature_num, cvSize(10,10), 3,
			found, 0, cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03 ), flag );
		flag |= CV_LKFLOW_PYR_A_READY;
	}

	memset( busy, 0, sizeof( busy ) );
	// drawing
	int k = 0;
	for ( int i = 0; i < feature_num; i++ ) {
		int flock = flock_num[i];

		/* If Pyramidal Lucas Kanade didn't really find the feature, skip it. */
		if ( flock == -1 || found[i] == 0 
			|| dist ( features1[i].x, features1[i].y, features2[i].x, features2[i].y ) > 60.0
			|| features2[i].x < 0 || features2[i].y < 0 
			|| features2[i].x >= image2->width || features2[i].y >= image2->height) continue;

		features2[k] = features2[i];
		flock_num[k] = flock;
		busy[(int)features2[k].x/SF][(int)features2[k].y/SF] = true;		
		DrawFeature( features1[i], features2[i], result, flock );		
		k++;
	}
	feature_num = k;

	// find features
	FindMotionArtifacts( gray1, gray2, gray3, motionFeatures, &motionFeaturesNumber );

	// get skin mask
	GetSkinMask( image2, tmp_gray );
	//cvSet( tmp_gray, cvScalarAll(255) );
	// and combine it with motion mask
	//cvAnd( gray1, tmp_gray, tmp_gray );
	//cvDilate( tmp_gray, tmp_gray );

	// map common mask on image to get moving skin parts
	cvSet( image1, cvScalarAll(200) );
	//cvFlip( gray1, 0, 0 );
	cvCopy( image2, image1, gray1 );
	
	DetectFace( gray2 );
	if ( trackerNumber > 0 )
		DetectHands( image1 );

	//Segment( image2, image1 );

	cvNamedWindow( "C", 2 );
	cvShowImage( "C", image1 );
	cvWaitKey(10);

	UpdateTracker( gray2, gray1 );
	DisplayTrackers( result, gray1 );

	CV_SWAP( pyr1, pyr2, swap_temp );
	CV_SWAP( features1, features2, swap_ftrs );

	cvReleaseImage( &eig );
	cvReleaseImage( &temp );
	cvReleaseImage( &gray1 );
	cvReleaseImage( &gray2 );
	cvReleaseImage( &gray3 );
	cvReleaseMat( &density );
	cvReleaseImage( &tmp_gray );
}

void ImageProcessor::DetectFace( IplImage* image )
{
	int scale = 1.5;
	CvMemStorage* storage = cvCreateMemStorage(0);
	IplImage* small_img = cvCreateImage( cvSize( cvRound(image->width/scale), cvRound(image->height/scale) ), 8, 1 );

	
	cvResize( image, small_img, CV_INTER_LINEAR );
	cvFlip( small_img );
	cvEqualizeHist( small_img, small_img );

	cvClearMemStorage( storage );

	/*-------------------------------------------------
	FINDING FACE HERE
	------------------------------------------------------*/
	if ( !cascade )
		cascade = (CvHaarClassifierCascade*)cvLoad( "haarcascade_frontalface_alt2.xml", 0, 0, 0 );
	if ( cascade ) {
		faces = cvHaarDetectObjects( small_img, cascade, storage, 1.1, 2, 0, cvSize(40, 40) );

		if ( faces )
			for( int i = 0; i < (faces ? faces->total>0 : 0); i++ ) {
				CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
				CvPoint center;
				int radius;
				center.x = cvRound((r->x + r->width*0.5)*scale);
				center.y = cvRound((r->y + r->height*0.5)*scale);
				radius = cvRound((r->width + r->height)*0.2);
				CreateTracker( center.x, center.y, 40, true );
			//		cvCircle( small_img, center, radius, CV_RGB(255,0,0) );
			}
	}

	cvReleaseImage( &small_img );
	cvReleaseMemStorage( &storage );
}



void ImageProcessor::DetectHands( IplImage* image )
{
	int scale = 1.4;
	CvMemStorage* storage = cvCreateMemStorage(0);
	IplImage* gray = cvCreateImage( cvGetSize(image), 8, 1 );
	IplImage* small_img = cvCreateImage( cvSize( cvRound(image->width/scale), cvRound(image->height/scale) ), 8, 1 );

	//cvCvtColor( image, gray, CV_BGR2GRAY );
	//cvResize( gray, small_img, CV_INTER_LINEAR );
	//cvEqualizeHist( small_img, small_img );

	cvClearMemStorage( storage );

	/*-------------------------------------------------
	FINDING HAND HERE
	------------------------------------------------------*/
	if ( !cascadeH )
		cascadeH = (CvHaarClassifierCascade*)cvLoad( "cascade_color.xml", 0, 0, 0 );
	if ( cascadeH ) {
		hands = cvHaarDetectObjects( image, cascadeH, storage, 1.2, 3, 0, cvSize(50, 50) );

		if ( hands )
			for( int i = 0; i < hands->total; i++ ) {
				CvRect* r = (CvRect*)cvGetSeqElem( hands, i );
				CvPoint center;
				int radius;
				center.x = cvRound((r->x + r->width*0.5)*scale);
				center.y = cvRound((r->y + r->height*0.5)*scale);
				radius = cvRound((r->width + r->height)*0.2);
				cvCircle( image, center, radius, CV_RGB(255,255,255), 3 );
				if ( radius > 5 && radius < 80 )
					CreateTracker( center.x, center.y, 35, false );
				//UpdateTracker( center.x, center.y, radius );
			}
	}

	cvReleaseImage( &gray );
	cvReleaseImage( &small_img );

	cvReleaseMemStorage( &storage );
}


void ImageProcessor::UpdateTracker( IplImage* gray, IplImage* mask )
{
	for ( int i = trackerNumber-1; i >= 0; i-- ) {
		int count = 0;

		double DX = 0.0, DY = 0.0;
		double minX = 1000.0, minY = 1000.0;
		double maxX = 0, maxY = 0;
		int DC = 0;

		// update feature positions
		// new tracker position is determined with new feature positions
		// moving fields have most priority
		// skin color next priority
		// and the rest have almost no influence
		for ( int j=0; j<feature_num; j++ ) {
			if ( flock_num[j] == -1 || flock_num[j] == i ) {
				int x = (int)features2[j].x;
				int y = (int)features2[j].y;

				if (!( x >=0 && y>=0 && x<gray->width && y<gray->height ) ) {
					break;
				}
				
				bool moves = mask->imageData[y*mask->width+x] > 0;
				bool skin = isSkin( cvGet2D( prevFrame, y, x ) ) > 0.0;
				
				int d = (int)dist( features1[j].x, features1[j].y, features2[j].x, features2[j].y );
				int value = (d>sqrt(2.0))*(2*skin+8*moves) + 1;
				
				DX += features2[j].x*value; 
				DY += features2[j].y*value;
				DC += value;
				
				flock_num[j] = i;
			}			
		}

		trackers[i].tracking = DC;

		if ( trackers[i].tracking > TRACKER_THRESHOLD ) {
			trackers[i].X = DX/DC;
			trackers[i].Y = DY/DC;

			/*for ( int j=0; j<feature_num; j++ ) {
				if ( flock_num[j] != i ) {
					double d = dist( trackers[i].X, trackers[i].Y, features2[j].x, features2[j].y );
					if ( d < trackers[i].radius ) {
						flock_num[j] = i;
					}
				}
			}*/

			// adding new features to fill the empty positions
			int fixed = feature_num;
			double alfa_step = 2*pi/ALFA_STEP_COUNT;
			int rad_step = trackers[i].radius/6;
			for ( int k=1; k<=6; k++ ) {
				for ( int j=0; j<ALFA_STEP_COUNT; j++ ) {
					int x = trackers[i].X+rad_step*k*cos(j*alfa_step);
					int y = trackers[i].Y+rad_step*k*sin(j*alfa_step);
					if ( x > 0 && x < currentFrame->width && y > 0 && y<currentFrame->height ) {
						if ( feature_num < MAX_COUNT-1 && !busy[x/SF][y/SF] ) {
							features2[feature_num].x = x;
							features2[feature_num].y = y;
							flock_num[feature_num] = i;
							feature_num++;
							busy[x/SF][y/SF] = true;
						}
					}
				}
			}
			cvFindCornerSubPix( gray, features2 + fixed - 1, feature_num-fixed,
                cvSize(10,10), cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));

			// checkin feature density
			// features must not be too close to each other
			// and not far from the center
			for ( int k=0; k<feature_num; k++ ) {
				if ( flock_num[k] == i ) {
					double d = 0;
					d = dist( features2[k].x, features2[k].y, trackers[i].X, trackers[i].Y );
					if ( d > 1.6*trackers[i].radius ) 
						flock_num[k] = -1;
					else if ( d > 1.3*trackers[i].radius ) {
						double alfa = atan2( features2[k].y-trackers[i].Y, features2[k].x-trackers[i].X )+pi;
						int x = trackers[i].X + 0.3*trackers[i].radius*cos(alfa);
						int y = trackers[i].Y + 0.3*trackers[i].radius*sin(alfa);

						if ( x>=0 && y>=0 && x<gray->width && y<gray->height && !busy[x/SF][y/SF] ) {
							features2[k].x = x;
							features2[k].y = y;
						}
						else
							flock_num[k] = -1;
					}
				}
			}
		}
		else
			RemoveTracker( i );
	}

	memset( busy, 0, sizeof(busy) );
	for ( int i=0; i<feature_num; i++ ) {
		int x = (int)features2[i].x/SF;
		int y = (int)features2[i].y/SF;
		if ( busy[x][y] )
			flock_num[i] = -1;
		else
			busy[x][y] = true;
	}
}

/*
*	Removes tracker
*/
void ImageProcessor::RemoveTracker( int num )
{
	for ( int i=0; i<feature_num; i++ ) {
		if ( flock_num[i] == num ) flock_num[i] = -1;
	}
	trackers[num] = trackers[--trackerNumber];
	for ( int i=0; i<feature_num; i++ ) {
		if ( flock_num[i] == trackerNumber ) flock_num[i] = num;
	}
}


void ImageProcessor::MoveTracker( int t_num, int X, int Y )
{
	int dx = X-trackers[t_num].X;
	int dy = Y-trackers[t_num].X;
	for ( int i=0; i<feature_num; i++ ) {
		if ( flock_num[i] == t_num ) {
			features2[i].x += dx;
			features2[i].y += dy;
		}
	}
	trackers[t_num].X = X;
	trackers[t_num].Y = Y;

	memset( busy, 0, sizeof(busy) );
	for ( int i=0; i<feature_num; i++ ) {
		int x = (int)features2[i].x/SF;
		int y = (int)features2[i].y/SF;
		if ( busy[x][y] )
			flock_num[i] = -1;
		else
			busy[x][y] = true;
	}
}


/*
*	Draw trackers on the image
*/
void ImageProcessor::DisplayTrackers( IplImage* image, IplImage* gray )
{
	for ( int i=0; i<trackerNumber; i++ ) {
		//int rad = trackers[i].radius;
		//cvCircle( image, cvPoint(trackers[i].X,trackers[i].Y), 20, CV_RGB(rad*10,rad*5,rad*4), 2 );
		CvScalar color = CV_RGB(255,0,0);
		if ( trackers[i].tracking > 0 )
			color = CV_RGB(0,255,0);

		if ( i == 0 ) {
		//	cvCircle( image, cvPoint(trackers[i].X, trackers[i].Y), trackers[i].radius, CV_RGB(0,0,255), 3 );
		}
		else {
			int x1 = (int)(trackers[i].X-trackers[i].radius);
			int y1 = (int)(trackers[i].Y-1.5*trackers[i].radius);
			int w = (int)(2*trackers[i].radius);
			int h = (int)(3*trackers[i].radius);

			cvSetImageROI( image, cvRect(x1,y1,w,h) );
			IplImage *tmp = cvCreateImage( cvGetSize(image), 8, 3 );
			cvCvtColor( image, tmp, CV_RGB2XYZ );
			cvResetImageROI( image );

			//cvSet( image, cvScalar(255,0,0), gray );

			/*cvNamedWindow( "ASD", 2 );
			cvShowImage("ASD",tmp);
			cvWaitKey(5);
			cvReleaseImage( &tmp );*/
			int defect = ExtractBlobDefects( tmp );
			
			//cvNamedWindow( "WIN" );
			//cvShowImage( "WIN", tmp );
			//cvWaitKey( 10 );
			//cvReleaseImage( &outputImage );
			cvReleaseImage( &tmp );
			
			cvRectangle( image, cvPoint(x1,y1), cvPoint(x1+w,y1+h), CV_RGB(0,0,255), 1 );
			//cvFloodFill( image, cvPoint(trackers[i].X,trackers[i].Y), CV_RGB(0,0,255), cvScalar(7,7,7), cvScalar(13,10,10) );
			cvCircle( image, cvPoint(trackers[i].X,trackers[i].Y-h/4), trackers[i].radius/3, CV_RGB((50+trackers[i].state*170)%256,(230-trackers[i].state*170)%256,50), 2 );

		}

		
	}
}

/*
*	Creating new tracker
*/
void ImageProcessor::CreateTracker( int X, int Y, int radius, bool isface )
{
	bool ok = trackerNumber<MAX_TRACKER_NUM;
	
	for ( int i=!isface; i<(!isface)*trackerNumber+isface; i++ ) {
		if ( dist( trackers[i].X, trackers[i].Y, X, Y ) < radius+trackers[i].radius ) {
			if ( isface ) {
				//MoveTracker( i, X, Y );				
			}
			else {
				trackers[i].X = (2*trackers[i].X+X)/3;
				trackers[i].Y = (2*trackers[i].Y+Y)/3;
			}
		//	if ( isface )
			
			trackers[i].radius = 1.3*radius;
			if ( trackers[i].radius > 60 )
				trackers[i].radius = 60;

			ok = false;
			break;
		}
	}

	if ( ok ) {
		trackers[trackerNumber].X = X;
		trackers[trackerNumber].Y = Y;
		if ( isface )
			trackers[trackerNumber].radius = 6;
		else
			trackers[trackerNumber].radius = radius;
		trackers[trackerNumber].tracking = 0;

		{
			int rad_step = trackers[trackerNumber].radius/6;
			double alfa_step = 2*pi/ALFA_STEP_COUNT;

			for ( int k=1; k<=6; k++ ) {
				for ( int i=0; i<ALFA_STEP_COUNT; i++ ) {
					int x = X+rad_step*k*cos(i*alfa_step);
					int y = Y+rad_step*k*sin(i*alfa_step);

					if ( x > 0 && x < currentFrame->width && y > 0 && y<currentFrame->height ) {
						if ( feature_num < MAX_COUNT-1 && !busy[x/SF][y/SF] ) {
							features2[feature_num].x = x;
							features2[feature_num].y = y;
							flock_num[feature_num] = trackerNumber;
							feature_num++;
							busy[x/SF][y/SF] = true;
						}
					}
				}
			}
		}
		trackerNumber++;
	}	
}

/*
*	Creating a mask with from image pixels with skin color values
*/
void ImageProcessor::GetSkinMask( IplImage* image, IplImage* mask )
{
	int index = 0;
	for ( int i=0; i<image->height; i++ ) {
		for ( int j=0; j<image->width; j++ ) {
			int p = 3*index;
			int b = image->imageData[p];
			int g = image->imageData[p+1];
			int r = image->imageData[p+2];
			mask->imageData[index] = 255*isSkin( cvGet2D( image, i, j ) );
			index++;
		}
	}


	//cvSmooth( mask, mask, CV_MEDIAN );
	//cvSmooth( mask, mask, CV_MEDIAN );
	//cvDilate( mask, mask, 0, 2 );
	//cvErode( mask, mask );
	/*cvDilate( mask, mask, 0, 2 );*/
}

void ImageProcessor::DrawFeature( CvPoint2D32f feature1, CvPoint2D32f feature2, IplImage* result, int param )
{
	int line_thickness = 1;

	CvPoint p,q;
	p.x = (int) feature1.x;
	p.y = (int) feature1.y;
	q.x = (int) feature2.x;
	q.y = (int) feature2.y;

	double angle = atan2( (double) p.y - q.y, (double) p.x - q.x );
	double hypotenuse = sqrt( square(p.y - q.y) + square(p.x - q.x) );// * 2 * cvmGet( density, p.y, p.x )/ max_dens;

	/*if ( hypotenuse <= sqrt(2.0) || hypotenuse > 60 )
	return;*/

	CvScalar line_color = CV_RGB(255,0,0);
	if ( hypotenuse > sqrt(2.0) ) {
		line_color = CV_RGB(0,0,255);
	}
	if ( hypotenuse > 4 ) {
		line_color = CV_RGB(0,255,0);
	}

	line_color = CV_RGB( (160+40*param)%256, (30+140*param)%256, 200*param%256 );


	/* Here we lengthen the arrow by a factor of three. */
#ifdef SHOW_ARROWS
	q.x = (int) (p.x + 3*hypotenuse * cos(angle));
	q.y = (int) (p.y + 3*hypotenuse * sin(angle));
	cvLine( result, p, q, line_color, line_thickness, CV_AA, 0 );
	q.x = (int) (p.x + 9 * cos(angle + pi / 4));
	q.y = (int) (p.y + 9 * sin(angle + pi / 4));
	cvLine( result, p, q, line_color, line_thickness, CV_AA, 0 );
	q.x = (int) (p.x + 9 * cos(angle - pi / 4));
	q.y = (int) (p.y + 9 * sin(angle - pi / 4));
	cvLine( result, p, q, line_color, line_thickness, CV_AA, 0 );
	cvLine( result, p, q, line_color, 2, CV_AA, 0 );
	//tmp_gray->imageData[p.y*tmp_gray->width+p.x] = 255;
#else
	cvCircle( result, p, 2, line_color, -1 );
#endif
}

void ImageProcessor::Segment( IplImage* image, IplImage* result )
{
    #define MAX_CLUSTERS 40
    CvRNG rng = cvRNG(-1);
    CvPoint ipt;

	cvNamedWindow( "clusters", 1 );

	cvCvtColor( image, result, CV_RGB2YCrCb );

	char key;
	int k, cluster_count = MAX_CLUSTERS;
	int i, sample_count = image->width*image->height;
	CvMat* points = cvCreateMat( sample_count, 5, CV_32FC1 );
	CvMat* clusters = cvCreateMat( sample_count, 1, CV_32SC1 );

	for( i = 0; i < sample_count; i++ )
	{
		double y = i/result->width;
		double x = i%result->width;
		CvScalar p = cvGet2D( result, (int)y, (int)x );
		x /= 1.0*result->width;
		y /= 1.0*result->height;
		p.val[0] /= 255.0;
		p.val[1] /= 255.0;
		p.val[2] /= 255.0;

		cvmSet( points, i, 0, y );
		cvmSet( points, i, 1, x );
		cvmSet( points, i, 2, p.val[0] );
		cvmSet( points, i, 3, p.val[1] );
		cvmSet( points, i, 4, p.val[2] );
	}

	cvKMeans2( points, cluster_count, clusters,
		cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 20, 1.0 ));

	for( i = 0; i < sample_count; i++ )
	{
		int cluster_idx = clusters->data.i[i];
		double y = i/image->width;
		double x = i%image->width;
		cvSet2D( image, y, x, cvScalar((50+2*cluster_idx)%256, (50+5*cluster_idx)%256, (50+8*cluster_idx)%256 ) );
	}

	cvReleaseMat( &points );
	cvReleaseMat( &clusters );

	cvShowImage( "clusters", image );
	cvWaitKey(5);
}

int ImageProcessor::ExtractBlobDefects(IplImage *image)
{
	//CBlobResult blobs1;
	//IplImage *chan1;

	//chan1 = cvCreateImage( cvGetSize(image), 8, 1 );
	//
	//cvCvtColor( image, chan1, CV_RGB2GRAY );
	//cvEqualizeHist( chan1, chan1 );

	//// Extract the blobs using a threshold of 100 in the image
	//blobs1 = CBlobResult( chan1, NULL, 100, true );

	//// discard the blobs with less area than 500 pixels
	//// ( the criteria to filter can be any class derived from COperadorBlob )
	//blobs1.Filter( blobs1, B_INCLUDE, CBlobGetArea(), B_GREATER, 500 );
	//
	//CBlob bigblob1;

	//// from the filtered blobs, get the blob with biggest perimeter
	//blobs1.GetNthBlob( CBlobGetPerimeter(), 0, bigblob1 );
	//
	////IplImage *outputImage;
	////outputImage = cvCreateImage( cvGetSize(tmp), 8, 3 );
	////cvMerge( tmp, tmp, tmp, 0, outputImage );
	//cvZero( chan1 );
	//
	//bigblob1.FillBlob( chan1, CV_RGB(255,255,255) );
	//
	//cvNamedWindow( "WIN" );
	//cvShowImage( "WIN", chan1 );
	//cvWaitKey( 10 );

	////blobWithLessArea.FillBlob( tmp, CV_RGB(0,0,0) );

	//// contours
	//CvMemStorage* storage = cvCreateMemStorage(0);
	//CvSeq* contours = 0;

	//// find contours
	////cvFindContours( image, storage, &contours, sizeof(CvContour),	CV_RETR_EXTERNAL , CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

	//// if any contours found
	//if ( contours ) {
	//	contours = cvApproxPoly( contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 4 , 1 ); // approximate
	//	//cvZero( image );
	//	//cvDrawContours( image, contours, CV_RGB(255,255,255), CV_RGB(255,255,255), 3, 1, CV_AA, cvPoint(0,0) );
	//}

	//cvReleaseMemStorage( &storage );

	return 0;

}



//IplImage *tmp = cvCreateImage( cvGetSize(currentFrame), 8, 1 );
//					cvZero( tmp );
//
//					CvMat* prob = cvCreateMat( currentFrame->height, currentFrame->width, CV_32FC1 );
//
//					cvZero( prob );
//					int found = 0;
//					for ( int k=0; k<feature_num; k++ ) {
//						cvCircle( tmp, cvPoint(motionFeatures[k].x,motionFeatures[k].y), 1, CV_RGB(255,255,255), 2 );
//						if ( distance[k] < 50.0 ) {
//							found++;
//							int MAXS = 100;
//							float step = 2*pi/MAXS;
//							float alfa = 0.0;
//							for ( int s=0; s<MAXS; s++ ) {
//								int XX = (int)(features2[k].x+distance[k]*cos(alfa));
//								int YY = (int)(features2[k].y+distance[k]*sin(alfa));
//								if ( XX >= 0 && YY >= 0 && XX < prob->width && YY < prob->height ) {
//									prob->data.fl[YY*prob->width+XX]++;
//									tmp->imageData[YY*prob->width+XX]+=5;
//								}
//								alfa += step;
//							}
//						}
//					}
//
//					cvReleaseMat( &prob );
//
//					/*cvZero( tmp );
//					cvCvtColor( image, tmp, CV_RGB2GRAY );
//					cvSobel( tmp, tmp, 1, 0 );
//					cvSobel( tmp, tmp, 0, 1 );*/
//
//					/*cvNamedWindow( "x");
//					cvShowImage( "x", tmp );
//					cvWaitKey(5);
//					cvReleaseImage( &tmp );*/
//
//					if ( found > 3 ) {
//						double min, max;
//						CvPoint max_loc;
//						cvMinMaxLoc( prob, &min, &max, NULL, &max_loc );
//
//						//cvLine( image, cvPoint( trackers[i].X, trackers[i].Y ), max_loc, CV_RGB(200,200,0), 2 );
//						trackers[i].X = max_loc.x;
//						trackers[i].Y = max_loc.y;
//					}