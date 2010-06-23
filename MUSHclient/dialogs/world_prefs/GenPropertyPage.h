#if !defined(AFX_GENPROPERTYPAGE_H__AB87C533_C53C_11D0_8EAF_00A0247B3BFD__INCLUDED_)
#define AFX_GENPROPERTYPAGE_H__AB87C533_C53C_11D0_8EAF_00A0247B3BFD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GenPropertyPage.h : header file
//

#include "..\..\MUSHclient.h"

typedef CTypedPtrMap <CMapStringToPtr, CString, CObject*> CObjectMap;

// comparator callback definition
typedef int (* t_CompareObjects) (const int iColumn, const CObject * item1, const CObject * item2);


struct t_gen_sort_param
{
  // initialise via constructor - makes sure we don't leave something out!
  t_gen_sort_param (const CObjectMap * arg_pObjectMap,
                    const int arg_sortkey,
                    const int arg_reverse,
                    const t_CompareObjects arg_CompareObjects) :
                    pObjectMap (arg_pObjectMap),
                    sortkey (arg_sortkey),
                    reverse (arg_reverse),
                    CompareObjects (arg_CompareObjects) {};

  const CObjectMap * pObjectMap;        // for looking the item up
  const int        sortkey;             // which key to use
  const int        reverse;             // reverse sort or not
  const t_CompareObjects CompareObjects;// how to compare them
};

/////////////////////////////////////////////////////////////////////////////
// CGenPropertyPage dialog

class CGenPropertyPage : public CPropertyPage
{
  DECLARE_DYNAMIC(CGenPropertyPage)

  public:
    CGenPropertyPage();
    CGenPropertyPage(const UINT nID);
    ~CGenPropertyPage();

    // Dialog Data
    //{{AFX_DATA(CGenPropertyPage)
    enum { IDD = IDD_TIP };
    // NOTE - ClassWizard will add data members here.
    // DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

    CString m_strObjectType;  // e.g "trigger", "alias" (used for name generation etc.)
    CObjectMap * m_ObjectMap; // map of objects (triggers, aliases etc.)

    unsigned long m_iMask;         // type of XML load/save
    __int64 m_nUpdateNumber;       // unique update number
    CMUSHclientDoc * m_doc;        // which document this belongs to
    int m_page_number;             // which page this is
    CFindInfo * m_pObjectFindInfo; // the findinfo structure
    CListCtrl * m_ctlList;         // the list control which displays the items
    CStatic   * m_ctlSummary;      // count of items shown

    // count of columns is used for arrays below
    int m_iColumnCount;               // how many columns there are

    // the next two items are arrays, based on "m_iColumnCount"
    int * m_iColWidth;             // width of each column
    CString * m_strColumnHeadings; // column headings
    int * m_iColJust;              // LVCFMT_LEFT or LVCFMT_RIGHT

    // for sorting the list
    int m_last_col;                    // which column to sort on
    BOOL m_reverse;                    // if true, reverse sort
    t_CompareObjects m_CompareObjects; // for comparing when sorting

    // set up variables for use later on
    void CGenPropertyPage::SetUpPage (CString strObjectType,
                                      CObjectMap * ObjectMap,
                                      CListCtrl * ctlList,
                                      CStatic * ctlSummary,
                                      t_CompareObjects CompareObjects,
                                      CFindInfo * pFindInfo,
                                      const unsigned long iMask);

    void OnAddItem(CDialog & dlg);         // adds a new item to the list
    void OnChangeItem(CDialog & dlg);      // changes an existing item
    void OnDeleteItem();                   // delete an item from the list
    void OnCopyItem();                     // copy an item to the clipboard in XML
    void OnPasteItem();                    // paste an item from the clipboard in XML
    void OnColumnclickItemList(NMHDR* pNMHDR, LRESULT* pResult);
    void LoadList();                       // load up list initially or after load from file
    bool EditFilterText (CString & sText); // edit the filter text

    // ================== start of virtual functions =======================

    // -- dialog management - initialise, load, unload, check if changed, get name

    // sets up for new dialog
    virtual void InitDialog (CDialog * pDlg) = 0;
    // move from item to dialog
    virtual void LoadDialog (CDialog * pDlg, CObject * pItem)  = 0;
     // move from dialog to item
    virtual void UnloadDialog (CDialog * pDlg, CObject * pItem) = 0; 
    // has the user changed anything?
    virtual bool CheckIfChanged (CDialog * pDlg, CObject * pItem) const = 0;
    // gets name from dialog
    virtual CString GetObjectName (CDialog * pDlg) const = 0;

    // -- object setup - create a new one, set its modification number, set its dispatch ID

    // create new item, e.g. trigger
    virtual CObject * MakeNewObject (void) = 0;
    // set modification number
    virtual void SetModificationNumber (CObject * pItem, __int64 m_nUpdateNumber) = 0;
    // set script dispatch ID
    virtual void SetDispatchID (CObject * pItem, const DISPID dispid) = 0;
    // name after creation
    virtual void SetInternalName (CObject * pItem, const CString strName) = 0;

    // -- get info about the object

    // get modification number
    virtual __int64 GetModificationNumber (CObject * pItem) const = 0;
    // get script subroutine name
    virtual CString GetScriptName (CObject * pItem) const = 0;
    // get item label
    virtual CString GetLabel (CObject * pItem) const = 0;
    // true if this was included
    virtual bool CheckIfIncluded (CObject * pItem) = 0;
    // true if this is executing a script
    virtual bool CheckIfExecuting (CObject * pItem) = 0;
    // is this a temporary item?
    virtual bool CheckIfTemporary (CObject * pItem) = 0;

    // -- list management

    // add one item to the list control
    virtual int AddItem (CObject * pItem, const int nItemNumber, const BOOL nItem) = 0;
    // is filtering enabled?
    virtual bool GetFilterFlag () = 0;
    // get the filter script
    virtual CString GetFilterScript () = 0;

    // ================== end of virtual functions =======================

    // add a single item - returns new item number
    int add_item (CObject * pItem, 
                  const CString * pstrObjectName,
                  const int nItem,
                  const BOOL bInsert);

    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CGenPropertyPage)
  protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
  protected:
    // Generated message map functions
    //{{AFX_MSG(CGenPropertyPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
    afx_msg void OnUpdateNeedSelection(CCmdUI* pCmdUI);
    afx_msg void OnUpdateNeedEntries(CCmdUI* pCmdUI);
    afx_msg void OnFind();     // do a find from start of buffer
    afx_msg void OnFindNext(); // find next occurrence

    DECLARE_MESSAGE_MAP()

  protected:
    static int CALLBACK CompareFunc (LPARAM lParam1,
                                     LPARAM lParam2,
                                     LPARAM lParamSort);

    // for finding
    static void InitiateSearch (const CObject * pObject,
                                CFindInfo & FindInfo);

    static bool GetNextLine (const CObject * pObject,
                             CFindInfo & FindInfo, 
                             CString & strLine);

    void DoFind (bool bAgain);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENPROPERTYPAGE_H__AB87C533_C53C_11D0_8EAF_00A0247B3BFD__INCLUDED_)
