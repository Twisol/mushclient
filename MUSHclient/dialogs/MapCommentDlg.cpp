// MapCommentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MapCommentDlg.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMapCommentDlg dialog

CMapCommentDlg::CMapCommentDlg(CWnd* pParent /*=NULL*/)
 : CDialog(CMapCommentDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CMapCommentDlg)
  //}}AFX_DATA_INIT
}

void CMapCommentDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CMapCommentDlg)
  DDX_MinMaxString(pDX, IDC_COMMENT, m_strComment);
  DDV_MinMaxString(pDX, m_strComment, 1, 100);
  //}}AFX_DATA_MAP

  if (!pDX->m_bSaveAndValidate)
    return;

   m_strComment.TrimLeft ();
   m_strComment.TrimRight ();

   int i = m_strComment.FindOneOf ("{}");
   if (i != -1)
     {
     ::UMessageBox(
         TFormat ("The comment may not contain the character \"%c\"", m_strComment [i])
         );
     DDX_Text(pDX, IDC_COMMENT, m_strComment);
     pDX->Fail();
     } // end of bad character
}


BEGIN_MESSAGE_MAP(CMapCommentDlg, CDialog)
  //{{AFX_MSG_MAP(CMapCommentDlg)
  // NOTE: the ClassWizard will add message map macros here
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapCommentDlg message handlers
