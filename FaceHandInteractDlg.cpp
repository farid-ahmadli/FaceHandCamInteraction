// FaceHandInteractDlg.cpp : implementation file
//

#include <windows.h>
#include "stdafx.h"
#include "FaceHandInteract.h"
#include "FaceHandInteractDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFaceHandInteractDlg dialog

CFaceHandInteractDlg::CFaceHandInteractDlg(CWnd* pParent /*=NULL*/)
: CDialog(CFaceHandInteractDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFaceHandInteractDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SHOCKWAVEFLASH1, m_FlashPlayer);
	DDX_Control(pDX, IDC_CUSTOM1, m_UserDisplay);
	DDX_Control(pDX, IDC_FPS, m_FramesPerSecond);
	DDX_Control(pDX, IDC_FPS2, m_DetectionTime);
	DDX_Control(pDX, IDC_FPS3, m_AcqusitionTime);
}

BEGIN_MESSAGE_MAP(CFaceHandInteractDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_STOP, &CFaceHandInteractDlg::OnBnClickedStop)
	ON_BN_CLICKED(ID_START, &CFaceHandInteractDlg::OnBnClickedStart)
	ON_WM_CLOSE()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


// CFaceHandInteractDlg message handlers

BOOL CFaceHandInteractDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Loading Flash Movie
	TCHAR path[500];
	GetCurrentDirectory( 500, path );
	LPWSTR filename = path;
	filename = wcscat( filename, L"\\carousel\\carousel3.swf" );
	m_FlashPlayer.LoadMovie( 0, filename );

	// Init Camera
	m_Capture = 0;
	m_CameraActive = false;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFaceHandInteractDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFaceHandInteractDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

UINT GetData(LPVOID lp) {

	TCHAR _fps[100];
	TCHAR _fps2[100];
	CString fps; 

	CFaceHandInteractDlg* dlg = (CFaceHandInteractDlg*)lp;
	while ( dlg->m_CameraActive ) {
		double t1 = (double)cvGetTickCount();	// initial time

		if ( !dlg->m_Capture ) {
#ifdef CAPTURE_FROM_AVI
			dlg->m_Capture = cvCaptureFromAVI("video.avi");
#else
			dlg->m_Capture = cvCaptureFromCAM(0);
#endif
		}
		if ( !cvGrabFrame( dlg->m_Capture ) ) {
			dlg->OnBnClickedStop();
			break;
		}

		IplImage *frame = cvRetrieveFrame( dlg->m_Capture );
		if ( !frame ) {
			dlg->OnBnClickedStop();
			break;
		}

		double t2 = (double)cvGetTickCount();	// after acqusition
		double f2 = (double)cvGetTickFrequency();	// frequency after acqusition

		int X,Y,EN;

		//if ( frame->origin != IPL_ORIGIN_TL ) cvFlip( frame, frame, 0 );
		dlg->m_ImgProc.DetectAndTrackHand( frame, &X, &Y, &EN );
		//if ( frame->origin != IPL_ORIGIN_TL ) cvFlip( frame, frame, 0 );
		cvFlip( frame, 0, 1 );
		double ratX = GetSystemMetrics( SM_CXFULLSCREEN )/frame->width;
		double ratY = GetSystemMetrics( SM_CYFULLSCREEN )/frame->height;
		LPPOINT p = new POINT;
		GetCursorPos( p );
		SetCursorPos( (p->x+ratX*(frame->width-X))/2, (p->y+ratY*(frame->height-Y))/2 );

		free( p );

		//LPRECT rect = new RECT;
		//dlg->m_FlashPlayer.ScreenToClient GetWindowRect( rect );

		double t3 = (double)cvGetTickCount();	// current time
		double f3 = (double)cvGetTickFrequency();	// current frequency

		dlg->m_FramesPerSecond.SetWindowText( CString("Total time: ") + _itow( (int)((t3-t1)/(f3*1000.)), _fps, 10 ) + CString("ms") );
		//dlg->m_AcqusitionTime.SetWindowText( CString("Acqusition time: ") + _itow( (int)((t2-t1)/(f2*1000.)) , _fps, 10 ) );

		dlg->m_AcqusitionTime.SetWindowText( CString("Image size: ") + _itow( frame->width, _fps, 10 ) + CString("x") + _itow( frame->height, _fps2, 10 ) );
		dlg->m_DetectionTime.SetWindowText( CString("Detection time: ") + _itow( (int)((t3-t2)/(f3*1000.)), _fps, 10 ) + CString("ms") );

		//cvCircle( frame, cvPoint(X,Y), 5, CV_RGB(255,0,0), 10 );
		dlg->m_UserDisplay.SetBitmap( frame, 3 );
	}

	cvReleaseCapture( &dlg->m_Capture );
	return 1;
}

void CFaceHandInteractDlg::OnBnClickedStop()
{
	m_CameraActive = false;
}

void CFaceHandInteractDlg::OnBnClickedStart()
{
	if ( !m_CameraActive ) {
		m_CameraActive = true;
		m_CameraThread = AfxBeginThread(GetData, this, THREAD_PRIORITY_NORMAL);	
	}
}

void CFaceHandInteractDlg::OnClose()
{
	OnBnClickedStop();
	CDialog::OnClose();
}

void CFaceHandInteractDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}
