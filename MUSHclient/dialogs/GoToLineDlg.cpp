// GoToLineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoToLineDlg.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoToLineDlg dialog

CGoToLineDlg::CGoToLineDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CGoToLineDlg::IDD, pParent),
    m_iLineNumber(0)
{}

void CGoToLineDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CGoToLineDlg)
  DDX_Text(pDX, IDC_LINE_NUMBER, m_iLineNumber);
  //}}AFX_DATA_MAP

  if (pDX->m_bSaveAndValidate && (m_iLineNumber < 1 || m_iLineNumber > m_iMaxLine))
    {
    ::UMessageBox(TFormat ("Line number must be in range 1 to %i", m_iMaxLine));
    DDX_Text(pDX, IDC_LINE_NUMBER, m_iLineNumber);
    pDX->Fail();
    }
}


BEGIN_MESSAGE_MAP(CGoToLineDlg, CDialog)
  //{{AFX_MSG_MAP(CGoToLineDlg)
  // NOTE: the ClassWizard will add message map macros here
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoToLineDlg message handlers
