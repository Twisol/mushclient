#pragma once

// password.h : header file
//

#include "..\MUSHclient.h"

/////////////////////////////////////////////////////////////////////////////
// CPasswordDialog dialog

class CPasswordDialog : public CDialog
{
  public:
    CPasswordDialog(CWnd* pParent = NULL);

    // Dialog Data
    //{{AFX_DATA(CPasswordDialog)
    enum { IDD = IDD_PASSWORD };
    CString m_character;
    CString m_password;
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPasswordDialog)
  protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
  protected:
    // Generated message map functions
    //{{AFX_MSG(CPasswordDialog)
    afx_msg void OnHelpbutton();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};
