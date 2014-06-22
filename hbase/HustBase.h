// HustBase.h : main header file for the HUSTBASE application
//

#if !defined(AFX_HUSTBASE_H__FC51556C_EEA4_4632_9CE6_1376CB171795__INCLUDED_)
#define AFX_HUSTBASE_H__FC51556C_EEA4_4632_9CE6_1376CB171795__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#define  BUFFER 200
/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp:
// See HustBase.cpp for the implementation of this class
//
#include "TreeList.h"
class CHustBaseApp : public CWinApp
{
public:
	
	static bool pathvalue;
	CHustBaseApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHustBaseApp)
	public:
	virtual BOOL InitInstance();
//	CString path;
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CHustBaseApp)
	afx_msg void OnAppAbout();
	afx_msg void OnCreateDB();
	afx_msg void OnOpenDB();
	afx_msg void OnDropDB();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HUSTBASE_H__FC51556C_EEA4_4632_9CE6_1376CB171795__INCLUDED_)

/*enum RC{
		CORRECT=0,
		SQL_SYNTAX=-10,
		//......suppose that a lot of errors here............
		CANNOT_FIND_TABLE
};*/

#ifndef RC_HH
#define RC_HH
typedef enum{
	SUCCESS=0,
	PF_EXIST,
	PF_FILEERR,
	PF_INVALIDNAME,
	PF_WINDOWS,
	PF_FHCLOSED,
	PF_FHOPEN,
	PF_PHCLOSED,
	PF_PHOPEN,
	PF_NOBUF,
	PF_EOF,
	PF_INVALIDPAGENUM,
	PF_NOTINBUF,
	PF_PAGEPINNED,
	RM_FHCLOSED,
	RM_FHOPENNED,
	RM_INVALIDRECSIZE,
	RM_INVALIDRID,
	RM_FSCLOSED,
	RM_NOMORERECINMEM,
	RM_FSOPEN,
	IX_IHOPENNED,
	IX_IHCLOSED,
	IX_INVALIDKEY,
	IX_NOMEM,
	RM_NOMOREIDXINMEM,
	IX_EOF,
	IX_SCANCLOSED,
	IX_ISCLOSED,
	IX_NOMOREIDXINMEM,
	IX_SCANOPENNED
}RC;
#endif
