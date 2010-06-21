// welcome.h : header file
//

#pragma once

#include "..\mushclient.h"
#include "..\StatLink.h"

/////////////////////////////////////////////////////////////////////////////
// CWelcomeDlg dialog

class CWelcomeDlg : public CDialog
{
  protected:
    // static controls with hyperlinks
    CStaticLink m_ChangesLink;

  public:
    CWelcomeDlg(CWnd* pParent = NULL);

    // Dialog Data
    //{{AFX_DATA(CWelcomeDlg)
    enum { IDD = IDD_WELCOME };
    CString m_strMessage;
    CString m_strChangeHistoryAddress;
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWelcomeDlg)
  protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
  protected:
    // Generated message map functions
    //{{AFX_MSG(CWelcomeDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};
