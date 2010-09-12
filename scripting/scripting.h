#ifndef MUSHCLIENT_SCRIPTING_SCRIPTING_H
#define MUSHCLIENT_SCRIPTING_SCRIPTING_H

extern "C"
  {
  #include "lua.h"
  #include "lualib.h"
  #include "lauxlib.h"
  }

#include "hostsite.h"
#include "paneline.h"

void LuaError (lua_State *L, 
               LPCTSTR strEvent = "Run-time error",
               LPCTSTR strProcedure = "",
               LPCTSTR strType = "",
               LPCTSTR strReason = "",
               CMUSHclientDoc * pDoc = NULL);


class ScriptEngineException : public exception
{
public:
  ScriptEngineException (const char* msg)
    : _msg(msg)
  {}
  
  ScriptEngineException (const CString& msg)
    : _msg((LPCTSTR) msg)
  {}
  
  virtual const char* what ()
  { return _msg; }

private:
  const char* _msg;
};

class IScriptEngine
{
public:
  virtual ~IScriptEngine() {};

  virtual DISPID GetDispid (const CString& routine) = 0;
  virtual DISPID GetLuaDispid (const CString& routine) = 0;

  virtual bool Parse (const CString& code, const CString& what) = 0;

  virtual bool Execute (DISPID& dispid,
                        LPCTSTR procedure,
                        const unsigned short reason_code,
                        LPCTSTR type,
                        LPCTSTR reason,
                        DISPPARAMS& params,
                        long& invocation_count,
                        COleVariant* result) = 0;

  virtual bool ParseLua (const CString& code, const CString& what) = 0;

  // returns true if script error
  virtual bool ExecuteLua (DISPID& dispid,
                           LPCTSTR procedure,
                           const unsigned short reason_code,
                           LPCTSTR type,
                           LPCTSTR reason,
                           list<double>& number_params,
                           list<string>& string_params,
                           long& invocation_count,
                           const t_regexp* regexp = NULL,
                           map<string, string>* table = NULL,
                           CPaneLine* paneline = NULL,
                           bool* result = NULL) = 0;

  // returns true if script error
  virtual bool ExecuteLua (DISPID& dispid,
                           LPCTSTR procedure,
                           const unsigned short reason_code,
                           LPCTSTR type,
                           LPCTSTR reason,
                           CString script_argument,
                           long& invocation_count,
                           CString& result) = 0;

  virtual bool IsLua () const = 0;
  virtual lua_State* LuaState () = 0;

  static IScriptEngine* Create (const CString& language, CMUSHclientDoc* pDoc);
};

class CScriptEngine : public CObject, public IScriptEngine
{
public:
  CScriptEngine (CMUSHclientDoc * pDoc, const CString strLanguage);
  ~CScriptEngine ();

  bool Parse (const CString & strCode, const CString & strWhat);

  DISPID GetDispid (const CString & strName);
  DISPID GetLuaDispid (const CString & strName);

  // returns true if script error
  bool Execute (DISPID & dispid,  // dispatch ID, will be set to DISPID_UNKNOWN on an error
                LPCTSTR szProcedure,  // eg. ON_TRIGGER_XYZ
                const unsigned short iReason,  // value for m_iCurrentActionSource
                LPCTSTR szType,   // eg. trigger, alias
                LPCTSTR szReason, // eg. trigger subroutine XXX
                DISPPARAMS & params,  // parameters
                long & nInvocationCount,  // count of invocations
                COleVariant * result    // result of call
                );
  bool ShowError (const HRESULT result, const CString& strMsg);

  bool ParseLua (const CString & strCode, const CString & strWhat);
  // returns true if script error
  bool ExecuteLua (DISPID & dispid,          // dispatch ID, will be set to DISPID_UNKNOWN on an error
                   LPCTSTR szProcedure,      // eg. ON_TRIGGER_XYZ
                   const unsigned short iReason,  // value for m_iCurrentActionSource
                   LPCTSTR szType,           // eg. trigger, alias
                   LPCTSTR szReason,         // eg. trigger subroutine XXX
                   list<double> & nparams,   // list of number parameters
                   list<string> & sparams,   // list of string parameters
                   long & nInvocationCount,  // count of invocations
                   const t_regexp * regexp = NULL,  // regular expression (for triggers, aliases)
                   map<string, string> * table = NULL, // map of other things
                   CPaneLine * paneline = NULL,        // and the line (for triggers)
                   bool * result = NULL);              // where to put result

  // returns true if script error
  bool ExecuteLua (DISPID & dispid,          // dispatch ID, will be set to DISPID_UNKNOWN on an error
                   LPCTSTR szProcedure,      // eg. ON_TRIGGER_XYZ
                   const unsigned short iReason,  // value for m_iCurrentActionSource
                   LPCTSTR szType,           // eg. trigger, alias
                   LPCTSTR szReason,         // eg. trigger subroutine XXX
                   CString strParam,         // string parameter
                   long & nInvocationCount,  // count of invocations
                   CString & result);        // where to put result

  // return value is return from call

  bool IsLua () const { return false; }
  lua_State* LuaState () { return NULL; }

private:
  IActiveScript       * m_IActiveScript;          // VBscript interface
  IActiveScriptParse  * m_IActiveScriptParse;     // parser
  CActiveScriptSite   * m_site;                   // our local site (world object)
  IDispatch           * m_pDispatch;              // script engine dispatch pointer

  CMUSHclientDoc      * m_pDoc;                   // related MUSHclient document

  CString               m_strLanguage;        // language, (vbscript, jscript, perlscript)

  bool CScriptEngine::Invoke (DISPID& dispid,
                              DISPPARAMS& params,
                              COleVariant* result,
                              LPCTSTR procedure,
                              LPCTSTR type,
                              LPCTSTR reason,
                              const unsigned short reason_code,
                              long& invocation_count);
};

int RegisterLuaRoutines (lua_State * L);
int DisableDLLs (lua_State * L);
#include "lua_helpers.h"

#endif // MUSHCLIENT_SCRIPTING_SCRIPTING_H
