// ButtonColour.h : header file
//

#pragma once

#include "..\..\MUSHclient.h"

/////////////////////////////////////////////////////////////////////////////
// CColourButton window

class CColourButton : public CButton
{
  public:
    CColourButton();
    virtual ~CColourButton();

    // Attributes
  public:
    COLORREF m_colour;
    COLORREF m_clipboardColour;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CColourButton)
  public:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    //}}AFX_VIRTUAL

    // Generated message map functions
  protected:
    //{{AFX_MSG(CColourButton)
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnPopupCopycolour();
    afx_msg void OnPopupPastecolour();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
