// PCRE or regexp

#include "stdafx.h"
#include "MUSHclient.h"
#include "doc.h"
#include "dialogs\RegexpProblemDlg.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


t_regexp::t_regexp(const char* pattern, const int flags)
  : m_program(NULL), m_extra(NULL), iTimeTaken(0),
    m_iCount(0), m_iExecutionError(0)
{
  this->Compile(pattern, flags);
}

t_regexp::~t_regexp ()
{ 
  if (m_program)
    pcre_free (m_program);
  if (m_extra)
    pcre_free (m_extra);
}

string t_regexp::GetWildcard (const int iNumber) const
{
  if (iNumber >= 0 && iNumber < m_iCount)
    return string (
      &m_sTarget.c_str () [m_vOffsets [iNumber * 2]],
      m_vOffsets [(iNumber * 2) + 1] - m_vOffsets [iNumber * 2]
      ).c_str ();
  else
    return "";
}

string t_regexp::GetWildcard (const string sName) const
{
  int iNumber;
  if (IsStringNumber (sName))
    iNumber = atoi (sName.c_str ());
  else if (m_program != NULL)
    iNumber = this->GetFirstSet (sName.c_str ());
  else
    return "";

  return GetWildcard (iNumber);
}

bool t_regexp::GetWildcardOffsets (const int iNumber, int& left, int& right) const
{
  if (iNumber >= 0 && iNumber < m_iCount)
  {
    left  = m_vOffsets [iNumber * 2];
    right = m_vOffsets [iNumber * 2 + 1];
    return true;
  }
  else
    return false;
}

bool t_regexp::GetWildcardOffsets (const string sName, int& left, int& right) const
{
  int iNumber;
  if (IsStringNumber (sName))
    iNumber = atoi (sName.c_str ());
  else if (m_program != NULL)
    iNumber = this->GetFirstSet (sName.c_str ());
  else
    return false;

  return GetWildcardOffsets (iNumber, left, right);
}

void t_regexp::Compile(const char* pattern, const int flags)
{
  const char *error = NULL;
  int erroroffset;

  pcre * program = pcre_compile(pattern, flags, &error, &erroroffset, NULL);
  if (!program)
    ThrowErrorException("Failed: %s at offset %d", Translate (error), erroroffset);

  // study it for speed purposes
  pcre_extra * extra = pcre_study(program, 0, &error);        
  if (error)
    {
    pcre_free(program);
    ThrowErrorException("Regexp study failed: %s", error);
    }

  // Remove old regexp
  pcre_free(this->m_program);
  pcre_free(this->m_extra);

  // remember program and extra stuff
  this->m_program = program;
  this->m_extra = extra;
  this->m_iExecutionError = 0; // reset the error code
  // leave the time taken alone

  // inspired by a suggestion by Twisol (to remove a hard-coded limit on the number of wildcards)
  int capturecount = 0;
  // how many captures did we get?
  this->GetInfo(PCRE_INFO_CAPTURECOUNT, &capturecount);
  // allocate memory for them
  this->m_vOffsets.resize ((capturecount + 1) * 3);  // add 1 for the whole expression
}

bool t_regexp::Execute(const char *string, const int start_offset)
{
  // exit if no regexp program to process (possibly because of previous error)
  if (this->m_program == NULL)
    return false;

  LARGE_INTEGER start;
  if (App.m_iCounterFrequency)
    QueryPerformanceCounter (&start);

  int options = App.m_bRegexpMatchEmpty ? 0 : PCRE_NOTEMPTY; // don't match on an empty string
  pcre_callout = NULL; // un-set the global pcre_callout() function pointer
  int count = pcre_exec(
      this->m_program, this->m_extra,
      string, strlen (string), start_offset,
      options, &this->m_vOffsets[0], this->m_vOffsets.size()
      );

  if (App.m_iCounterFrequency)
    {
    LARGE_INTEGER finish;
    QueryPerformanceCounter (&finish);
    this->iTimeTaken += finish.QuadPart - start.QuadPart;
    }

  if (count == PCRE_ERROR_NOMATCH)
    return false;  // no match
  else if (count < 0)
    {
    this->m_iExecutionError = count; // remember reason
    ThrowErrorException (TFormat (
        "Error executing regular expression: %s",
        this->ErrorCodeToString (count)
        ));
    }

  this->m_iExecutionError = 0; // success!

  // if, and only if, we match we will save the matching string, the count
  // and offsets, so we can extract the wildcards later on
  this->m_sTarget = string; // for extracting wildcards
  this->m_iCount  = count;  // ditto

  return true; // match
}

LONGLONG t_regexp::TimeTaken() const
{
  return this->iTimeTaken;
}

string t_regexp::LastTarget() const
{
  return this->m_sTarget;
}

int t_regexp::LastError() const
{
  return this->m_iExecutionError;
}

string t_regexp::LastErrorString() const
{
  return this->ErrorCodeToString(this->m_iExecutionError);
}

int t_regexp::GetInfo(int what, void* where) const
{
  return pcre_fullinfo(this->m_program, this->m_extra, what, where);
}

int t_regexp::MatchedCapturesCount() const
{
  return this->m_iCount;
}

bool t_regexp::DupNamesAllowed() const
{
  unsigned long int options = 0;
  this->GetInfo(PCRE_INFO_OPTIONS, &options);
  if (options & PCRE_DUPNAMES)
    return true;

  int jchanged = false;
  this->GetInfo(PCRE_INFO_JCHANGED, &jchanged);
  if (jchanged)
    return true;

  return false;
}

bool t_regexp::CheckPattern(const CString strRegexp, const int iOptions,
                                   const char** error, int* errorOffset)
{
  pcre * program = pcre_compile(strRegexp, iOptions, error, errorOffset, NULL);
  if (program != NULL)
    {
    pcre_free(program);
    return true;
    }
  else
    return false;
}


string t_regexp::ErrorCodeToString(const int code)
{
  switch (code)
    {
    case PCRE_ERROR_NOMATCH:        return Translate ("No match");       
    case PCRE_ERROR_NULL:           return Translate ("Null");           
    case PCRE_ERROR_BADOPTION:      return Translate ("Bad option");     
    case PCRE_ERROR_BADMAGIC:       return Translate ("Bad magic");      
    case PCRE_ERROR_UNKNOWN_OPCODE: return Translate ("Unknown Opcode"); 
    case PCRE_ERROR_NOMEMORY:       return Translate ("No Memory");      
    case PCRE_ERROR_NOSUBSTRING:    return Translate ("No Substring");   
    case PCRE_ERROR_MATCHLIMIT:     return Translate ("Match Limit");    
    case PCRE_ERROR_CALLOUT:        return Translate ("Callout");        
    case PCRE_ERROR_BADUTF8:        return Translate ("Bad UTF8");       
    case PCRE_ERROR_BADUTF8_OFFSET: return Translate ("Bad UTF8 Offset");
    case PCRE_ERROR_PARTIAL:        return Translate ("Partial");        
    case PCRE_ERROR_BADPARTIAL:     return Translate ("Bad Partial");    
    case PCRE_ERROR_INTERNAL:       return Translate ("Internal");       
    case PCRE_ERROR_BADCOUNT:       return Translate ("Bad Count");      
    case PCRE_ERROR_DFA_UITEM:      return Translate ("Dfa Uitem");      
    case PCRE_ERROR_DFA_UCOND:      return Translate ("Dfa Ucond");      
    case PCRE_ERROR_DFA_UMLIMIT:    return Translate ("Dfa Umlimit");    
    case PCRE_ERROR_DFA_WSSIZE:     return Translate ("Dfa Wssize");     
    case PCRE_ERROR_DFA_RECURSE:    return Translate ("Dfa Recurse");    
    case PCRE_ERROR_RECURSIONLIMIT: return Translate ("Recursion Limit");
    case PCRE_ERROR_NULLWSLIMIT:    return Translate ("Null Ws Limit");  
    case PCRE_ERROR_BADNEWLINE:     return Translate ("Bad Newline");    
    default:                        return Translate ("Unknown PCRE error");
    }
}


/*************************************************
*    Find first set of multiple named strings    *
*************************************************/

// adapted from get_first_set in pcre_get.c

/* This function allows for duplicate names in the table of named substrings.
It returns the number of the first one that was set in a pattern match.

Arguments:
  name   the name of the capturing substring

Returns:       the number of the first that is set,
               or the number of the last one if none are set,
               or a negative number on error
*/

typedef unsigned char uschar;

int t_regexp::GetFirstSet(const char* name) const
{
  if (!this->DupNamesAllowed())
    return pcre_get_stringnumber(this->m_program, name);

  char *first, *last;
  int entrysize = pcre_get_stringtable_entries(this->m_program, name, &first, &last);
  if (entrysize <= 0)
    return entrysize;

  for (uschar* entry = (uschar *)first; entry <= (uschar *)last; entry += entrysize)
    {
    int n = (entry[0] << 8) + entry[1];
    if (this->m_vOffsets[n*2] >= 0)
      return n;
    }

  return (first[0] << 8) + first[1];
}

// checks a regular expression, raises a dialog if bad
bool CheckRegularExpression (const CString strRegexp, const int iOptions)
{
  const char *error = NULL;
  int erroroffset;

  if (t_regexp::CheckPattern(strRegexp, iOptions, &error, &erroroffset))
    return true; // good

  CRegexpProblemDlg dlg;
  dlg.m_strErrorMessage = Translate (error);
  dlg.m_strErrorMessage += ".";   // end the sentence
  // make first character upper-case, so it looks like a sentence. :)
  dlg.m_strErrorMessage.SetAt (0, toupper (dlg.m_strErrorMessage [0]));

  dlg.m_strColumn = TFormat ("Error occurred at column %i.", erroroffset + 1);
  dlg.m_strText = strRegexp;
  dlg.m_strText += ENDLINE;
  if (erroroffset > 0)
    dlg.m_strText += CString ('-', erroroffset - 1);
  dlg.m_strText += '^';
  dlg.m_iColumn = erroroffset + 1;

  dlg.DoModal ();
  return false; // bad
}