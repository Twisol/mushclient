// welcome1.h : header file
//

#pragma once

#include "..\mushclient.h"
#include "..\StatLink.h"

/////////////////////////////////////////////////////////////////////////////
// CWelcome1Dlg dialog

class CWelcome1Dlg : public CDialog
{
  protected:
    // static controls with hyperlinks
    CStaticLink m_ChangesLink;

  public:
    CWelcome1Dlg(CWnd* pParent = NULL);

    // Dialog Data
    //{{AFX_DATA(CWelcome1Dlg)
    enum { IDD = IDD_WELCOME1 };
    CString m_strMessage1;
    CString m_strMessage2;
    CString m_strChangeHistoryAddress;
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWelcome1Dlg)
  protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
  protected:
    // Generated message map functions
    //{{AFX_MSG(CWelcome1Dlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};
