// CG: This file added by 'Tip of the Day' component.

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTipDlg dialog

class CTipDlg : public CDialog
{
  public:
    CTipDlg(CWnd* pParent = NULL);
    ~CTipDlg();

    // Dialog Data
    //{{AFX_DATA(CTipDlg)
    BOOL    m_bStartup;
    CString m_strTip;
    //}}AFX_DATA

    FILE* m_pStream;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTipDlg)
  protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
  protected:
    // Generated message map functions
    //{{AFX_MSG(CTipDlg)
    afx_msg void OnNextTip();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    //}}AFX_MSG

    void GetNextTipString(CString& strNext);

    DECLARE_MESSAGE_MAP()
};

