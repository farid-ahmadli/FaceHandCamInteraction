// FaceHandInteractDlg.h : header file
//

#pragma once
#include "shockwaveflash1.h"
#include "bitmapviewer.h"
#include "imageprocessor.h"
#include "afxwin.h"


// CFaceHandInteractDlg dialog
class CFaceHandInteractDlg : public CDialog
{
// Construction
public:
	CFaceHandInteractDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FACEHANDINTERACT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

// Public Variables
public:
	CShockwaveflash1	m_FlashPlayer;
	CBitmapViewer		m_UserDisplay;
	CWinThread*			m_CameraThread;
	CvCapture*			m_Capture;
	ImageProcessor		m_ImgProc;
	bool				m_CameraActive;
	CStatic				m_FramesPerSecond;

// Public Implementation
public:
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedStart();
	afx_msg void OnClose();
	
public:
	CStatic m_DetectionTime;
public:
	CStatic m_AcqusitionTime;
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};
