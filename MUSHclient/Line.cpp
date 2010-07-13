#include "stdafx.h"
#include "MUSHclient.h"

#include "doc.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/*

  CLine is the class for "lines" from the MUSH, which are shown in the output
  window.

  By making them a class we can produce a collection class which is a linked
  list of CLine's.

  Also, the constructor allocates memory for the text of the line and its first
  style item, and the destructor de-allocates the line's memory, and the style list.

*/

CLine::CLine (const long nLineNumber, 
              const unsigned int nWrapColumn,
              const unsigned short iFlags,      
              const COLORREF iForeColour,
              const COLORREF iBackColour,
              const bool bUnicode)
  : hard_return(false), len(0), last_space(-1),
    m_theTime(CTime::GetCurrentTime()), m_nLineNumber(nLineNumber),
    iMemoryAllocated(nWrapColumn * (bUnicode ? 4 : 1)), // allocate 4 bytes per character for UTF-8
    flags(0) // no special flags yet (ie. normal output line)
{
  QueryPerformanceCounter (&m_lineHighPerformanceTime);

#ifdef USE_REALLOC
  text = (char *) malloc (iMemoryAllocated);
  if (!text)
    AfxThrowMemoryException ();
#else
  try
    {
    text = new char [iMemoryAllocated];
    }
  catch (bad_alloc&)
    {
    AfxThrowMemoryException ();
    }
#endif

  // have at least one style item in the list
  CStyle * pStyle = NEWSTYLE;
  pStyle->iFlags = iFlags;
  pStyle->iForeColour = iForeColour;
  pStyle->iBackColour = iBackColour;

  styleList.AddTail (pStyle);
}

// CLine destructor

CLine::~CLine ()
{
#ifdef USE_REALLOC
  free (text);
#else
  delete [] text;
#endif

  // delete styles list
  for (POSITION pos = styleList.GetHeadPosition(); pos; )
    DELETESTYLE (styleList.GetNext (pos));

  styleList.RemoveAll();
}

// for tracking down style allocation errors
CStyle * GetNewStyle (const char * filename, const long linenumber)
  {
  CStyle * pNewStyle = new CStyle;
  TRACE3 ("new CStyle at %p at file %s line %ld\n",
      pNewStyle, filename, linenumber
      );
  return pNewStyle;
}

void DeleteStyle (CStyle * pStyle, const char * filename, const long linenumber)
{
  TRACE3 ("delete CStyle at %p at file %s line %ld\n",
      pStyle, filename, linenumber
      );
  delete pStyle;
}
