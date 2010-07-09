
/* regexp.h - Definitions etc. for regexp(3) routines. */

#ifndef __REGEXP_H
#define __REGEXP_H



#define MAX_WILDCARDS 10

// This is Perl-Compatible Regular Expressions 

/*************************************************
*       Perl-Compatible Regular Expressions      *
*************************************************/

/* Copyright (c) 1997-2000 University of Cambridge */

#define PCRE_EXP_DECL extern

#include "pcre\pcre.h"

// compiled regular expression type
class t_regexp 
{
  public:
    t_regexp (const char* pattern, const int flags = 0);
    ~t_regexp ();

    string GetWildcard (const int iNumber) const;  // numbered wildcards
    string GetWildcard (const string sName) const; // named wildcards

    void Compile(const char* pattern, const int flags = 0);
    bool Execute(const char *string, const int start_offset = 0);

    LONGLONG TimeTaken() const;

    string LastTarget() const;

    int LastError() const;
    string LastErrorString() const;

    int GetInfo(int what, void* where) const;
    bool DupNamesAllowed() const;

    static bool CheckPattern(const CString strRegexp, const int iOptions,
                             const char** error = NULL, int* errorOffset = NULL);

    int m_iCount;           // count of matches
    vector<int> m_vOffsets; // pairs of offsets from match

  private:
    pcre * m_program;     // the program itself
    pcre_extra * m_extra; // extra stuff for speed

    // the string we last matched on (to extract wildcards from)
    string m_sTarget;

    int m_iExecutionError;  // error code if failed execution

    LONGLONG iTimeTaken;

    int GetFirstSet(const char* name) const; // for duplicate named wildcards

    static string ErrorCodeToString(const int code);
};

bool CheckRegularExpression (const CString strRegexp, const int iOptions);

#endif  // #ifndef __REGEXP_H
