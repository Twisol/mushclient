// PCRE or regexp

#include "stdafx.h"
#include "doc.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

t_regexp::t_regexp(const char* pattern, const int flags)
  : m_program(NULL), m_extra(NULL), iTimeTaken(0),
    m_iCount(0), m_iExecutionError(0),
    m_iMatchAttempts(0)
{
  this->Compile(pattern, flags);
}

t_regexp::t_regexp (const t_regexp& other)
  : iTimeTaken(other.iTimeTaken), m_iCount(other.m_iCount),
    m_sTarget(other.m_sTarget), m_vOffsets(other.m_vOffsets),
    m_iExecutionError(other.m_iExecutionError)
{
  this->AcquirePattern(other.m_program, other.m_extra);
}

t_regexp::~t_regexp ()
{
  this->ReleasePattern();
}

t_regexp& t_regexp::operator=(const t_regexp& other)
{
  this->ReleasePattern();
  this->AcquirePattern(other.m_program, other.m_extra);

  this->m_iExecutionError = other.m_iExecutionError;
  this->iTimeTaken        = other.iTimeTaken;
  this->m_iCount          = other.m_iCount;
  this->m_sTarget         = other.m_sTarget;
  this->m_vOffsets        = other.m_vOffsets;

  return *this;
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
  if (m_program == NULL)
    return "";

  return GetWildcard (this->GetFirstSet (sName.c_str ()));
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
  if (m_program == NULL)
    return false;

  return GetWildcardOffsets (this->GetFirstSet (sName.c_str ()), left, right);
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

  this->ReleasePattern(); // Release previous pattern
  this->AcquirePattern(program, extra);

  this->m_iExecutionError = 0; // Start with no error
  this->m_iMatchAttempts = 0;
  // leave the time taken alone
}

bool t_regexp::Execute(const char *string, const int start_offset)
{
  // exit if no regexp program to process (possibly because of previous error)
  if (this->m_program == NULL)
    return false;

  // inspired by a suggestion by Twisol (to remove a hard-coded limit on the number of wildcards)
  int capturecount = 0;
  // how many captures are in the pattern?
  this->GetInfo(PCRE_INFO_CAPTURECOUNT, &capturecount);
  // allocate memory for them
  vector<int> offsets((capturecount + 1) * 3); // we always get offset 0 - the whole match

  LARGE_INTEGER start;
  if (App.m_iCounterFrequency)
    QueryPerformanceCounter (&start);

  int options = App.m_bRegexpMatchEmpty ? 0 : PCRE_NOTEMPTY; // don't match on an empty string
  pcre_callout = NULL; // un-set the global pcre_callout() function pointer
  int count = pcre_exec(
      this->m_program, this->m_extra,
      string, strlen (string), start_offset,
      options, &offsets[0], offsets.size()
      );

  if (App.m_iCounterFrequency)
    {
    LARGE_INTEGER finish;
    QueryPerformanceCounter (&finish);
    this->iTimeTaken += finish.QuadPart - start.QuadPart;
    }

  this->m_iMatchAttempts += 1; // how many times did we try to match?

  if (count == PCRE_ERROR_NOMATCH)
    return false; // no match  - don't save matching string etc.
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

  // store the captures
  this->m_vOffsets.clear();
  this->m_vOffsets.insert(this->m_vOffsets.begin(), offsets.begin(), offsets.end());

  return true; // match
}

LONGLONG t_regexp::TimeTaken() const
{
  return this->iTimeTaken;
}

long t_regexp::MatchAttempts() const
{
  return this->m_iMatchAttempts;
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



bool t_regexp::CheckPattern(const char* strRegexp, const int iOptions,
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

/* This function allows for duplicate names in the table of named substrings.
It returns the number of the first one that was set in a pattern match.

Arguments:
  name   the name of the capturing substring

Returns:       the number of the first that is set,
               or the number of the last one if none are set,
               or a negative number on error
*/

// adapted from get_first_set in pcre_get.c
int t_regexp::GetFirstSet(const char* name) const
{
  typedef unsigned char uschar;

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

void t_regexp::AcquirePattern(pcre* program, pcre_extra* extra)
{
  this->ReleasePattern();
  this->m_program = program;
  this->m_extra   = extra;
  pcre_refcount(program, 1);
}

void t_regexp::ReleasePattern()
{
  if (this->m_program != NULL)
    return;

  if (pcre_refcount(this->m_program, -1) == 0)
    {
    pcre_free(this->m_program);
    this->m_program = NULL;
    pcre_free(this->m_extra);
    this->m_extra   = NULL;
    }
}
