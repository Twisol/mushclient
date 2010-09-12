#include "stdafx.h"
#include "lua_scriptengine.h"

#include "..\MUSHclient.h"
#include "..\doc.h"
#include "..\luacom\luacom.h"

#include "..\pcre\config.h"
#include "..\pcre\pcre_internal.h"

extern "C"
  {
  LUALIB_API int luaopen_rex (lua_State *L);
  LUALIB_API int luaopen_bits (lua_State *L);
  LUALIB_API int luaopen_compress (lua_State *L);
  LUALIB_API int luaopen_bc (lua_State *L);
  LUALIB_API int luaopen_lsqlite3 (lua_State *L);
  LUALIB_API int luaopen_lpeg (lua_State *L);
  }
extern int luaopen_progress_dialog (lua_State *L);
static int luacom_open_glue (lua_State *L);

set<string> LuaFunctionsSet;
set<string> LuaTablesSet;

static void BuildLuaFunctions (lua_State * L);
static void BuildLuaTables (lua_State * L);


LuaScriptEngine::LuaScriptEngine (CMUSHclientDoc* doc)
  : L(NULL), m_doc(doc)
{
  L = MakeLuaState();
  if (!L)
    throw ScriptEngineException ("Unable to create Lua script engine");

  luaL_openlibs (L); // open all standard Lua libraries

  // call as Lua function because we need the environment
  lua_pushcfunction(L, RegisterLuaRoutines);
  lua_pushstring(L, DOCUMENT_STATE);  /* push address */
  lua_pushlightuserdata(L, (void *)m_doc);    /* push value */
  lua_call(L, 2, 0);

  luaopen_rex (L);
  luaopen_bits (L);
  luaopen_compress (L); // compression/utils
  luaopen_progress_dialog (L);
  luaopen_bc (L);
  luaopen_lsqlite3 (L);

  // open lpeg library
  lua_pushcfunction (L, luaopen_lpeg);
  lua_call (L, 0, 0);

  lua_settop(L, 0); // clear stack

  // for function-name completion, and help
  if (LuaFunctionsSet.empty ())
    {
    BuildLuaFunctions (L);
    BuildLuaTables (L);
    }

  // unless they explicitly enable it, remove ability to load DLLs
  DisableDLLs (L);

  // add luacom to package.preload
  lua_getglobal (L, LUA_LOADLIBNAME);  // package table

  if (lua_istable (L, -1))    // if it exists and is a table
    {
    lua_getfield (L, -1, "preload");  // get preload table inside it

    if (lua_istable (L, -1))   // preload table exists
      {
      lua_pushcfunction(L, luacom_open_glue);   // luacom open
      lua_setfield(L, -2, "luacom");          
      } // have package.preload table

    lua_pop (L, 1);   // get rid of preload table from stack
    } // have package table

  lua_pop (L, 1);   // get rid of package table from stack

  // this is so useful I am adding it in (see check.lua)
  ParseLua ( \
  "function check (result)  \
    if result ~= error_code.eOK then\
      error (error_desc [result] or \
             string.format (\"Unknown error code: %i\", result), 2) end; end", 
  "Check function");

  // preliminary sand-box stuff
  m_doc->m_iCurrentActionSource = eLuaSandbox;
  ParseLua ((LPCTSTR) App.m_strLuaScript, "Sandbox");

  m_doc->m_iCurrentActionSource = eUnknownActionSource;

  lua_settop(L, 0);   // clear stack
}

LuaScriptEngine::~LuaScriptEngine ()
{
  if (L)
    {
    lua_close (L);
    L = NULL;
    }
}

bool LuaScriptEngine::Parse (const CString& code, const CString& what)
{
  // safety check ;)
  if (!L)
    return true;

  LARGE_INTEGER start, finish;

  if (App.m_iCounterFrequency)
    QueryPerformanceCounter (&start);

  // note - an error here is a *compile* error
  if (luaL_loadbuffer(L, code, code.GetLength (), what))
    {
    LuaError (L, "Compile error", "", "", "", m_doc);
    return true;
    }

  int error = CallLuaWithTraceBack (L, 0, 0);

  // note - an error here is a *runtime* error
  if (error)
    {
    LuaError (L, "Run-time error", "", "", "", m_doc);
    return true;
    }

  lua_settop(L, 0);   // clear stack

// -----------------

  if (App.m_iCounterFrequency)
    {
    QueryPerformanceCounter (&finish);
    m_doc->m_iScriptTimeTaken += finish.QuadPart - start.QuadPart;
    }

  return false;
}

bool LuaScriptEngine::ExecuteLua (DISPID& dispid,
                                  LPCTSTR procedure,
                                  const unsigned short reason_code,
                                  LPCTSTR type,
                                  LPCTSTR reason,
                                  CString script_argument,
                                  long& invocation_count,
                                  CString& result)
{
  // safety check ;)
  if (!L)
    return false;

  // don't do it if previous problems
  if (dispid == DISPID_UNKNOWN)
    return false;

  lua_settop (L, 0);  // start with empty stack

  LARGE_INTEGER start, finish;

  m_doc->Trace (TFormat ("Executing %s script \"%s\"", type, procedure));

  if (App.m_iCounterFrequency)
    QueryPerformanceCounter (&start);
             
  unsigned short iOldStyle = m_doc->m_iNoteStyle;
  m_doc->m_iNoteStyle = NORMAL;    // back to default style

  if (!GetNestedFunction (L, procedure, true))
    {
    dispid = DISPID_UNKNOWN;   // stop further invocations
    return true;    // error return
    }

  lua_pushstring (L, script_argument);    // the solitary argument

  if (reason_code != eDontChangeAction)
    m_doc->m_iCurrentActionSource = reason_code;

  int error = CallLuaWithTraceBack (L, 1, LUA_MULTRET);

  if (reason_code != eDontChangeAction)
    m_doc->m_iCurrentActionSource = eUnknownActionSource;

  m_doc->m_iNoteStyle = iOldStyle;

  if (error)
    {
    dispid = DISPID_UNKNOWN;   // stop further invocations
    LuaError (L, "Run-time error", procedure, type, reason, m_doc);
    return true;    // error return
    }

  invocation_count++;   // count number of times used

  if (App.m_iCounterFrequency)
    {
    QueryPerformanceCounter (&finish);
    m_doc->m_iScriptTimeTaken += finish.QuadPart - start.QuadPart;
    }

  if (lua_gettop (L) > 0 && lua_isstring (L, 1))
    {
    // get result

    size_t textLength;
    const char * text = luaL_checklstring (L, 1, &textLength);

    result = CString (text, textLength);
    }

  lua_settop (L, 0);  // discard any results now

  return false;   // no error
}

bool LuaScriptEngine::ExecuteLua (DISPID& dispid,
                                  LPCTSTR procedure,
                                  const unsigned short reason_code,
                                  LPCTSTR type,
                                  LPCTSTR reason,
                                  list<double>& nparams,
                                  list<string>& sparams,
                                  long& invocation_count,
                                  const t_regexp* regexp,
                                  map<string, string>* table,
                                  CPaneLine* paneline,
                                  bool* result)
{
  // safety check ;)
  if (!L)
    return false;

  // don't do it if previous problems
  if (dispid == DISPID_UNKNOWN)
    return false;

  lua_settop (L, 0);  // start with empty stack

  LARGE_INTEGER start, finish;

  m_doc->Trace (TFormat ("Executing %s script \"%s\"", type, procedure));

  if (App.m_iCounterFrequency)
    QueryPerformanceCounter (&start);

  unsigned short iOldStyle = m_doc->m_iNoteStyle;
  m_doc->m_iNoteStyle = NORMAL;    // back to default style

  if (!GetNestedFunction (L, procedure, true))
    {
    dispid = DISPID_UNKNOWN;   // stop further invocations
    return true;    // error return
    }

  int paramCount = 0;

  // push all supplied number parameters
  for (list<double>::const_iterator niter = nparams.begin ();
       niter != nparams.end ();
       niter++)
     lua_pushnumber (L, *niter);

  paramCount += nparams.size ();

  // push all supplied string parameters
  for (list<string>::const_iterator siter = sparams.begin ();
       siter != sparams.end ();
       siter++)
     lua_pushlstring (L, siter->c_str (), siter->size ());

  paramCount += sparams.size ();

// if we have a regular expression, push the wildcards

  if (regexp)
    {
    int i;
    int ncapt;
    int namecount;
    unsigned char *name_table;
    int name_entry_size;
    unsigned char *tabptr;
    lua_newtable(L);                                                            
    paramCount++;   // we have one more parameter to the call
    pcre_fullinfo(regexp->m_program, regexp->m_extra, PCRE_INFO_CAPTURECOUNT, &ncapt);

    for (i = 0; i <= ncapt; i++) 
      {
      string wildcard (regexp->GetWildcard (i));
      lua_pushlstring (L, wildcard.c_str (), wildcard.size ());
      lua_rawseti (L, -2, i);
    }

    /* now do named subpatterns  */
    pcre_fullinfo(regexp->m_program, regexp->m_extra, PCRE_INFO_NAMECOUNT, &namecount);
    if (namecount > 0)
      {
      pcre_fullinfo(regexp->m_program, regexp->m_extra, PCRE_INFO_NAMETABLE, &name_table);
      pcre_fullinfo(regexp->m_program, regexp->m_extra, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
      tabptr = name_table;
      set<string> found_strings;
      for (i = 0; i < namecount; i++, tabptr += name_entry_size) 
        {
        int n = (tabptr[0] << 8) | tabptr[1];
        int j = n * 2;
        const unsigned char * name = tabptr + 2;
        // if duplicates were possible then ...
        if ((regexp->m_program->options & (PCRE_DUPNAMES | PCRE_JCHANGED)) != 0)
          {
          // this code is to ensure that we don't find a match (eg. mob = Kobold)
          // and then if duplicates were allowed, replace Kobold with false.

          string sName = (LPCTSTR) name;

          // for duplicate names, see if we already added this name
          if (found_strings.find (sName) != found_strings.end ())
            {
            // do not replace if this one is out of range
            if (n < 0 || n > ncapt)
              continue;
            } // end of duplicate
          else
            found_strings.insert (sName);
          }

        lua_pushstring (L, (LPCTSTR) name);
        if (n >= 0 && n <= ncapt) 
          {
          string wildcard (regexp->GetWildcard (n));
          lua_pushlstring (L, wildcard.c_str (), wildcard.size ());
          }
        else
          lua_pushnil (L);  /* n out of range */
        lua_settable (L, -3);
        
        }   // end of wildcard loop
      } // end of having named wildcards
    } // end of having a regexp

  if (table)
    {
    lua_newtable (L);
    paramCount++;   // we have one more parameter to the call

    // add each item to the table
    for (map<string, string>::const_iterator iter = table->begin ();
         iter != table->end ();
         iter++)
           {
           lua_pushstring (L, iter->first.c_str ());
           lua_pushstring (L, iter->second.c_str ());
           lua_settable(L, -3);
           }  // end of doing each one

    } // end of having an optional table

  if (paneline)
    {
    lua_newtable(L);
    paramCount++;   // we have one more parameter to the call
    int i = 1;          // style run number

    for (CPaneStyleVector::iterator style_it = paneline->m_vStyles.begin (); 
         style_it != paneline->m_vStyles.end (); 
         style_it++, i++)
      {
      lua_newtable(L);                                                            
      MakeTableItem     (L, "text",         (*style_it)->m_sText); 
      MakeTableItem     (L, "length",       (*style_it)->m_sText.length ());  
      MakeTableItem     (L, "textcolour",   (*style_it)->m_cText);  
      MakeTableItem     (L, "backcolour",   (*style_it)->m_cBack);  
      MakeTableItem     (L, "style",        (*style_it)->m_iStyle); 

      lua_rawseti (L, -2, i);  // set table item as number of style
      }

    }   // end of having an optional style run thingo

  if (reason_code != eDontChangeAction)
    m_doc->m_iCurrentActionSource = reason_code;

  int error = CallLuaWithTraceBack (L, paramCount, LUA_MULTRET);

  if (reason_code != eDontChangeAction)
    m_doc->m_iCurrentActionSource = eUnknownActionSource;

// -----------------

  m_doc->m_iNoteStyle = iOldStyle;

  if (error)
    {
    dispid = DISPID_UNKNOWN;   // stop further invocations
    LuaError (L, "Run-time error", procedure, type, reason, m_doc);
    return true;    // error return
    }

  invocation_count++;   // count number of times used

  if (App.m_iCounterFrequency)
    {
    QueryPerformanceCounter (&finish);
    m_doc->m_iScriptTimeTaken += finish.QuadPart - start.QuadPart;
    }

  if (result)
    {
    *result = true;

    int i = lua_toboolean (L, paramCount + 1);

    // if a boolean result wanted, return it

    if (lua_gettop (L) > 0)
      {
      if (lua_type (L, 1) == LUA_TBOOLEAN)
        *result = lua_toboolean (L, 1);
      else
        *result = lua_tonumber (L, 1);  // I use number rather than boolean
                                       // because 0 is considered true in Lua
      }
    }   // end of result wanted

  lua_settop (L, 0);  // discard any results now
  
  return false;   // no error
}

bool LuaScriptEngine::Execute (DISPID& dispid,
                               LPCTSTR procedure,
                               const unsigned short reason_code,
                               LPCTSTR type,
                               LPCTSTR reason,
                               DISPPARAMS& params,
                               long& invocation_count,
                               COleVariant* result)
{
  list<double> nparams;
  list<string> sparams;
  bool r;
  bool status = ExecuteLua (dispid,
                            procedure,
                            reason_code,
                            type, 
                            reason, 
                            nparams,
                            sparams, 
                            invocation_count,
                            NULL, NULL, NULL, &r);

  if (result)
    {
    result->vt = VT_BOOL;
    result->boolVal = r; 
    }
  return status;
}

DISPID LuaScriptEngine::GetDispid (const CString& routine)
{
  // if known 1 is flag, otherwise DISPID_UNKNOWN
  return (L && FindLuaFunction (L, routine)) ? 1 : DISPID_UNKNOWN;
}


static void BuildOneLuaFunction (lua_State* L, string sTableName)
{
  lua_settop (L, 0); // get rid of stuff lying around

  // get global table
  lua_getglobal (L, sTableName.c_str ());

  const int table = 1;

  if (!lua_istable (L, table))
    return;  // aha! caught you!

  // standard Lua table iteration
  for (lua_pushnil (L); lua_next (L, table) != 0; lua_pop (L, 1))
    {
    // extract function names
    if (lua_isfunction (L, -1))
      {
      string sName (luaL_checkstring (L, -2));

      // don't add stuff like __index
      if (sName [0] != '_')
        {
        string sFullName;

        // global table will not have _G prefix
        if (sTableName != "_G")
          {
          sFullName= sTableName;
          sFullName += ".";
          }

        // now the name
        sFullName += sName;

        // add to set
        LuaFunctionsSet.insert (sFullName);
        }  // not prefixed by _
      }   // end if function
    }  // end for loop

  lua_pop (L, 1); // get rid of global table now
} // end of BuildOneLuaFunction

static void BuildOneLuaTable (lua_State* L, string sTableName)
{
  lua_settop (L, 0); // get rid of stuff lying around

  // get global table
  lua_getglobal (L, sTableName.c_str ());  

  const int table = 1;

  if (!lua_istable (L, table))
    return; // aha! caught you!

  // standard Lua table iteration
  for (lua_pushnil (L); lua_next (L, table) != 0; lua_pop (L, 1))
    {
    if (lua_isstring (L, -2))
      {
      // "table . entry", eg. sendto.world
      LuaTablesSet.insert (sTableName + "." + lua_tostring (L, -2));
      } // if string key
    } // end for loop

  lua_pop (L, 1); // get rid of global table now
} // end of BuildOneLuaTable

// for world completion, and help lookup, find all functions in the following tables
static void BuildLuaFunctions (lua_State* L)
  {
  const char* table_names [] = {
      "_G",
      "string",
      "package",
      "os",
      "io",
      "bc",
      "progress",
      "bit",
      "rex",
      "utils",
      "table",
      "math",
      "debug",
      "coroutine",
      "lpeg",
      "sqlite3",

      "" // end of table marker
    };

  // add all functions from each table
  for (int i = 0; table_names [i] [0]; i++)
    BuildOneLuaFunction (L, table_names [i]);

  // experimental function - don't show it
  LuaFunctionsSet.erase ("newproxy");

  } // end of BuildLuaFunctions

// for word completion, find all entries in the following tables
static void BuildLuaTables (lua_State* L)
  {
  const char* table_names [] = {
    "trigger_flag",
    "alias_flag",
    "timer_flag",
    "custom_colour",
    "error_code",
    "sendto",
    "miniwin",

    "" // end of table marker
  };

  // add all functions from each table
  for (int i = 0; table_names [i] [0]; i++)
    BuildOneLuaTable (L, table_names [i]);
} // end of BuildLuaTables

// sigh - luacom_open returns void
static int luacom_open_glue (lua_State *L)
{
  luacom_open (L);
  return 0;
 }