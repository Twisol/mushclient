#ifndef MUSHCLIENT_SCRIPTING_LUA_SCRIPTENGINE_H
#define MUSHCLIENT_SCRIPTING_LUA_SCRIPTENGINE_H

#include "scripting.h"

#define DOCUMENT_STATE "mushclient.document"

class LuaScriptEngine : public IScriptEngine
{
public:
  LuaScriptEngine (CMUSHclientDoc* doc);
  ~LuaScriptEngine ();

  virtual DISPID GetDispid (const CString& routine);
  virtual DISPID GetLuaDispid (const CString& routine)
  {return this->GetDispid (routine);}

  virtual bool Parse (const CString& code, const CString& what);
  virtual bool ParseLua (const CString& code, const CString& what)
  {return this->Parse (code, what);}

  virtual bool Execute (DISPID& dispid,
                        LPCTSTR procedure,
                        const unsigned short reason_code,
                        LPCTSTR type,
                        LPCTSTR reason,
                        DISPPARAMS& params,
                        long& invocation_count,
                        COleVariant* result);

  // returns true if script error
  virtual bool ExecuteLua (DISPID& dispid,
                           LPCTSTR procedure,
                           const unsigned short reason_code,
                           LPCTSTR type,
                           LPCTSTR reason,
                           list<double>& nparams,
                           list<string>& sparams,
                           long& invocation_count,
                           const t_regexp* regexp = NULL,
                           map<string, string>* table = NULL,
                           CPaneLine* paneline = NULL,
                           bool* result = NULL);

  // returns true if script error
  virtual bool ExecuteLua (DISPID& dispid,
                           LPCTSTR procedure,
                           const unsigned short reason_code,
                           LPCTSTR type,
                           LPCTSTR reason,
                           CString script_argument,
                           long& invocation_count,
                           CString& result);

  virtual bool IsLua () const {return true;}

  lua_State* LuaState () {return L;}
private:
  CMUSHclientDoc* m_doc;
  lua_State* L; // easier to refer to a lot without the m_ prefix
};

#endif // MUSHCLIENT_SCRIPTING_LUA_SCRIPTENGINE_H