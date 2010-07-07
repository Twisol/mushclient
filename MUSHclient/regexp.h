
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
                 
// for duplicate named wildcards
int njg_get_first_set(const pcre *code, const char *stringname, const int *ovector);

// compiled regular expression type

class t_regexp 
{
  public:
    t_regexp (const char* pattern, const int flags = 0);
    ~t_regexp ();

    string GetWildcard (const int iNumber) const; // numbered wildcards
    string GetWildcard (const string sName) const; // named wildcards

    int Execute(const char *string, const int start_offset = 0);

    static bool CheckPattern(const CString strRegexp, const int iOptions,
                             const char** error = NULL, int* errorOffset = NULL);

    // pairs of offsets from match
    vector<int> m_vOffsets;

    // count of matches
    int m_iCount;
    // the string we match on (to extract wildcards from)
    string m_sTarget;
    // the program itself
    pcre * m_program;

    // extra stuff for speed
    pcre_extra * m_extra;

    int m_iExecutionError;  // error code if failed execution

    LONGLONG iTimeTaken;
};

__declspec(deprecated)
t_regexp * regcomp(const char *exp, const int options = 0);

bool CheckRegularExpression (const CString strRegexp, const int iOptions);

#endif  // #ifndef __REGEXP_H
