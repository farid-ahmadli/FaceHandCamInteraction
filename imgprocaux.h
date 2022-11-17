#ifndef IMGPROAUX_H
#define IMGPROAUX_H

#include "math.h"

static const double pi = 3.14159265358979323846;

static bool colors_loaded = false;
static double skin[256][256];

inline static double square(int a)
{
	return a * a;
}

inline static double square(double a)
{
	return a * a;
}

inline static int maximum( int a, int b )
{
	return a > b ? a : b;
}

inline static int minimum( int a, int b )
{
	return a < b ? a : b;
}

inline static double dist( double x1, double y1, double x2, double y2 )
{
	return sqrt( square(x1-x2) + square(y1-y2) );
}

inline static double isSkin( CvScalar p )
{
	if ( !colors_loaded ) {
		FILE *f = fopen( "skin.dat", "r" );
		
		if ( f ) {
			for ( int r=0; r<256; r++ ) {
				for ( int g=0; g<256; g++ ) {
					fscanf( f, "%lf", &skin[r][g] );
				}
			}
			fclose( f );
		}
		
		colors_loaded = true;
	}
	double sum = p.val[0]+p.val[1]+p.val[2];

	if ( sum < 100.0 || p.val[2] < 50 /* || p.val[0]>p.val[2]*/ )
		return 0.0;

	/*double dRG = abs(p.val[2]-p.val[1]);
	double dGB = abs(p.val[1]-p.val[0]);
	double diff = dRG-dGB;
	if ( diff<-15 || diff > 30 )
		return 0.0;*/

	int r = (int)(255*p.val[2]/sum);
	int g = (int)(255*p.val[1]/sum);
	
	return skin[r][g]/50.0>0.1;
}

#endif