#if !defined(AFX_BITMAPVIEWER_H__BF97F412_279F_11D4_A2F6_0048543D92F7__INCLUDED_)
#define AFX_BITMAPVIEWER_H__BF97F412_279F_11D4_A2F6_0048543D92F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BitmapViewer.h : header file
//

#pragma once

#include <cv.h>
#include <highgui.h>

#pragma comment(lib, "cv.lib")
#pragma comment(lib, "cxcore.lib")
#pragma comment(lib, "highgui.lib")
#pragma comment(lib, "ml.lib")

#define BITMAPVIEWER_CLASSNAME    _T("MFCBitmapViewerCtrl")  // Window class name

/////////////////////////////////////////////////////////////////////////////
// CBitmapViewer window

class CBitmapViewer : public CWnd
{
// Construction
public:
	CBitmapViewer();

// Attributes
    BOOL SetBitmap(IplImage* newImage, int nChannels);

// Operations
	BOOL Create(CWnd* pParentWnd, const RECT& rect, UINT nID, DWORD dwStyle = WS_VISIBLE);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBitmapViewer)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBitmapViewer();
protected:
    BOOL RegisterWindowClass();

protected:
	BITMAPINFO* bmi;
    BITMAPINFOHEADER* bmih;
    RGBQUAD* palette;
    unsigned int buffer[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*256]; 
	IplImage* TheImage;
	bool Selected;
	CvVideoWriter* Video;

	// Generated message map functions
protected:
	//{{AFX_MSG(CBitmapViewer)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BITMAPVIEWER_H__BF97F412_279F_11D4_A2F6_0048543D92F7__INCLUDED_)
