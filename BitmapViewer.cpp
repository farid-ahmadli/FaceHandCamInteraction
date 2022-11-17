// BitmapViewer.cpp : implementation file
//



#include "stdafx.h"
#include <sys/stat.h> 
#include "BitmapViewer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBitmapViewer

CBitmapViewer::CBitmapViewer()
{
	RegisterWindowClass();

	// Init the image
	TheImage = NULL;
	Video = NULL;
	Selected = false;
}

CBitmapViewer::~CBitmapViewer()
{
	if (Video)
		cvReleaseVideoWriter( &Video );
}

// Register the window class if it has not already been registered.
BOOL CBitmapViewer::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, BITMAPVIEWER_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = BITMAPVIEWER_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}


BEGIN_MESSAGE_MAP(CBitmapViewer, CWnd)
	//{{AFX_MSG_MAP(CBitmapViewer)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBitmapViewer message handlers

void CBitmapViewer::OnPaint() 
{
	CRect rect;
	GetClientRect(rect);
	if ( TheImage == NULL ) {
		TheImage = cvCreateImage( cvSize(rect.Width(), rect.Height()), IPL_DEPTH_8U, 1 );
		cvSetZero(TheImage);
		if ( Selected )
			cvRectangle( TheImage, cvPoint(2,2), cvPoint(TheImage->width-3, TheImage->height-3), cvScalar(255, 255, 255), 5 );
		//cvLine( TheImage, cvPoint(0, 0), cvPoint(rect.Width()-1, rect.Height()-1), cvScalar(255) );
		//cvLine( TheImage, cvPoint(0, rect.Height()-1), cvPoint(rect.Width()-1, 0), cvScalar(255) );
	}
	if ( TheImage != NULL ) {
		
		int width = TheImage->width;
		int height = TheImage->height;

		//Initialize the BMP display buffer
		bmi = (BITMAPINFO*)buffer;
		bmih = &(bmi->bmiHeader);
		memset( bmih, 0, sizeof(*bmih));
		bmih->biSize = sizeof(BITMAPINFOHEADER);
		bmih->biWidth = width;
		bmih->biHeight = height;
		bmih->biPlanes = 1;
		bmih->biCompression = BI_RGB;
		bmih->biBitCount = 8 * TheImage->nChannels;
		palette = bmi->bmiColors;
		if (TheImage->nChannels == 1)
		{
			for( int i = 0; i < 256; i++ )
			{
				palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
				palette[i].rgbReserved = 0;
			}
		}

		if ( Selected ) {
			cvRectangle( TheImage, cvPoint(2,2), cvPoint(TheImage->width-3, TheImage->height-3), cvScalar(255, 255, 255), 5 );
		}

		CPaintDC dc(this);
		CDC* pDC = &dc;		

		int res = StretchDIBits(
			pDC->GetSafeHdc(), //dc
			0, //x dest
			0, //y dest
			width, //x dest dims
			height, //y dest dims
			0, // src x
			0, // src y
			width, // src dims
			height, // src dims
			TheImage->imageData, // array of DIB bits
			(BITMAPINFO*)bmi, // bitmap information
			DIB_RGB_COLORS, // RGB or palette indexes
			SRCCOPY); // raster operation code
	}	

}

BOOL CBitmapViewer::OnEraseBkgnd(CDC* pDC) 
{
	// If we have an image then don't perform any erasing, since the OnPaint
	// function will simply draw over the background
	if ( TheImage != NULL )
		return TRUE;

	// Obviously we don't have a bitmap - let the base class deal with it.
	return CWnd::OnEraseBkgnd(pDC);
}

void CBitmapViewer::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ( Selected ) {
		Selected = false;
	}
	else {
		char s[20];
		for ( int i=0; ; i++ ) {
			sprintf(s, "video%d.avi", i);

			struct stat stFileInfo;
			if ( stat( s, &stFileInfo ) != 0 ) 
				break;
		}

		Video = cvCreateVideoWriter( s , -1, 10, cvGetSize(TheImage) );

		Selected = true;
	}
	RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
}

void CBitmapViewer::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	// In our case this is not needed - yet - so just drop through to
	// the base class
	CWnd::PreSubclassWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CBitmapViewer methods

BOOL CBitmapViewer::Create(CWnd* pParentWnd, const RECT& rect, UINT nID, DWORD dwStyle /*=WS_VISIBLE*/)
{
	return CWnd::Create(BITMAPVIEWER_CLASSNAME, _T(""), dwStyle, rect, pParentWnd, nID);
}

BOOL CBitmapViewer::SetBitmap(IplImage* newImage, int nChannels)
{
	CRect rect;
	GetClientRect(rect);
	cvReleaseImage(&TheImage);
	TheImage = cvCreateImage( cvSize(rect.Width(), rect.Height()), IPL_DEPTH_8U, nChannels );
	cvResize( newImage, TheImage, CV_INTER_AREA );
	if ( Selected && Video ) {
		cvFlip(TheImage, 0, 1);
		cvWriteFrame( Video, TheImage );
		cvFlip(TheImage);
	}
	else {
		cvReleaseVideoWriter( &Video );
		//cvFlip(TheImage, 0, -1);		
	}
	RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
	
	return true;
}

