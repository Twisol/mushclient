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


static const int MAX_PCRE_WILDCARDS = 1000;


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
    iNumber = njg_get_first_set (m_program, sName.c_str (), &m_vOffsets [0]);
  else
    iNumber = PCRE_ERROR_NOSUBSTRING;
  return GetWildcard (iNumber);
}

void t_regexp::Compile(const char* pattern, const int flags)
{
  const char *error = NULL;
  int erroroffset;

  pcre * program = pcre_compile(pattern, flags, &error, &erroroffset, NULL);
  if (!program)
    ThrowErrorException("Failed: %s at offset %d", Translate (error), erroroffset);

  // Ensure that there aren't too many captures in the regexp.
  int capturecount = 0;
  pcre_fullinfo(program, NULL, PCRE_INFO_CAPTURECOUNT, &capturecount);
  if (capturecount > MAX_PCRE_WILDCARDS + 1)
    {
    pcre_free(program);
    ThrowErrorException (Translate ("Too many substrings in regular expression"));
    }

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
}

bool t_regexp::Execute(const char *string, const int start_offset)
{
  int capturecount = 0;
  pcre_fullinfo(this->m_program, NULL, PCRE_INFO_CAPTURECOUNT, &capturecount);
  int* offsets = new int[(capturecount + 1) * 3];

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
      options, offsets, NUMITEMS (offsets)
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
        Convert_PCRE_Runtime_Error (count)
        ));
    }

  // if, and only if, we match we will save the matching string, the count
  // and offsets, so we can extract the wildcards later on
  this->m_sTarget = string; // for extracting wildcards
  this->m_iCount  = count;  // ditto
  this->m_vOffsets.clear ();

  // only need first 2/3 of offsets
  copy (offsets, &offsets [count * 2], back_inserter (this->m_vOffsets));
  delete[] offsets;
  return true; // match
}

LONGLONG t_regexp::TimeTaken() const
{
  return this->iTimeTaken;
}

int t_regexp::LastError() const
{
  return this->m_iExecutionError;
}

string t_regexp::LastTarget() const
{
  return this->m_sTarget;
}

int t_regexp::GetInfo(int what, void* where) const
{
  return pcre_fullinfo(this->m_program, this->m_extra, what, where);
}

bool t_regexp::DupNamesAllowed() const
{
  unsigned long int options = 0;
  this->GetInfo(PCRE_INFO_OPTIONS, &options);
  if ((options & PCRE_DUPNAMES) != 0)
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