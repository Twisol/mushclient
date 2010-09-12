#include "stdafx.h"
#include "..\mainfrm.h"
#include "..\MUSHclient.h"
#include "..\doc.h"
#include "..\dialogs\ScriptErrorDlg.h"
#include <io.h>  // for popen
#include <fcntl.h>  // for popen
#include "..\pcre\config.h"
#include "..\pcre\pcre_internal.h"
#include "..\luacom\luacom.h"

#include "lua_scriptengine.h"

// send some code to Lua to be parsed
bool CScriptEngine::ParseLua (const CString & strCode, const CString & strWhat)
{
  throw ScriptEngineException ("Not supported.");
}

// For Lua we will simply use the DISPID as a flag to indicate if we found
// the function or not, to speed up subsequent calls (calls to non-existent 
// functions). Also, if the function later has an error we set the DISPID
// to DISPID_UNKNOWN as a flag to not call it continuously.

// Version 3.75+ supports dotted functions (eg. string.gsub)

DISPID CScriptEngine::GetLuaDispid (const CString & strName)
{
  throw ScriptEngineException ("Not supported.");
}


void LuaError (lua_State *L, 
                LPCTSTR strEvent,
                LPCTSTR strProcedure,
                LPCTSTR strType,
                LPCTSTR strReason,
                CMUSHclientDoc * pDoc)
  {    
  CScriptErrorDlg dlg;

  if (!strlen (strProcedure) == 0)
    {
    dlg.m_strCalledBy = "Function/Sub: ";
    dlg.m_strCalledBy += strProcedure;
    dlg.m_strCalledBy += " called by ";
    dlg.m_strCalledBy += strType;
    dlg.m_strCalledBy += ENDLINE;
    dlg.m_strCalledBy += "Reason: ";
    dlg.m_strCalledBy += strReason;
    }
  else
    {
    dlg.m_strCalledBy = "Immediate execution";

/*    dlg.m_strDescription += ENDLINE;
    dlg.m_strDescription += "Line in error: ";
    dlg.m_strDescription += ENDLINE;
    dlg.m_strDescription += bstr;
*/
    }

  dlg.m_strEvent = strEvent;
  dlg.m_strDescription = lua_tostring(L, -1);
  lua_settop(L, 0);   // clear stack

  dlg.m_strRaisedBy = "No active world";

  if (pDoc)
    {
    if (pDoc->m_CurrentPlugin)
      {
      dlg.m_strRaisedBy = "Plugin: " +  pDoc->m_CurrentPlugin->m_strName;
      dlg.m_strRaisedBy += " (called from world: " + pDoc->m_mush_name + ")";
      }
    else 
      dlg.m_strRaisedBy = "World: " + pDoc->m_mush_name;
    }

  if (!pDoc || !pDoc->m_bScriptErrorsToOutputWindow)
    {
    if (pDoc)
      dlg.m_bHaveDoc = true;
    dlg.DoModal ();
    if (pDoc && dlg.m_bUseOutputWindow)
      {
      pDoc->m_bScriptErrorsToOutputWindow = true;
      pDoc->SetModifiedFlag (TRUE);
      }

    }
  else
    {
    pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, strEvent);
    pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strRaisedBy);
    pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strCalledBy);
    pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strDescription);

// show bad lines?

    bool bImmediate = true;
    int nLine = 0;

    if (dlg.m_strDescription.Left (18) == "[string \"Plugin\"]:")
      {
      bImmediate = false;
      nLine = atoi (dlg.m_strDescription.Mid (18));
      }
    else if (dlg.m_strDescription.Left (23) == "[string \"Script file\"]:")
      {
      bImmediate = false;
      nLine = atoi (dlg.m_strDescription.Mid (23));
      }

    if (!bImmediate)
      pDoc->ShowErrorLines (nLine);
    
    }  // end of showing in output window

  }   // end of LuaError

// returns true if error
bool CScriptEngine::ExecuteLua (DISPID & dispid,  // dispatch ID, will be set to DISPID_UNKNOWN on an error
                                LPCTSTR szProcedure,      // eg. ON_TRIGGER_XYZ
                                const unsigned short iReason,  // value for m_iCurrentActionSource
                                LPCTSTR szType,           // eg. trigger, alias
                                LPCTSTR szReason,         // eg. trigger subroutine XXX
                                list<double> & nparams,   // list of number parameters
                                list<string> & sparams,   // list of string parameters
                                long & nInvocationCount,  // count of invocations
                                const t_regexp * regexp,  // regular expression (for triggers, aliases)
                                map<string, string> * table,   // map of other things
                                CPaneLine * paneline,     // and the line (for triggers)
                                bool * result)            // where to put result
{
  throw ScriptEngineException ("Not supported.");
} // end of CScriptEngine::ExecuteLua 

// returns true if script error
bool CScriptEngine::ExecuteLua (DISPID & dispid,          // dispatch ID, will be set to DISPID_UNKNOWN on an error
                               LPCTSTR szProcedure,      // eg. ON_TRIGGER_XYZ
                               const unsigned short iReason,  // value for m_iCurrentActionSource
                               LPCTSTR szType,           // eg. trigger, alias
                               LPCTSTR szReason,         // eg. trigger subroutine XXX
                               CString strParam,         // string parameter
                               long & nInvocationCount,  // count of invocations
                               CString & result)         // where to put result
{
  throw ScriptEngineException ("Not supported.");
} // end of CScriptEngine::ExecuteLua 


void GetTracebackFunction (lua_State *L)
  {
  lua_pushliteral (L, LUA_DBLIBNAME);     // "debug"    
  lua_rawget      (L, LUA_GLOBALSINDEX);    // get debug library   

  if (!lua_istable (L, -1))
    {
    lua_pop (L, 2);   // pop result and debug table
    lua_pushnil (L);
    return;
    }

  // get debug.traceback
  lua_pushstring(L, "traceback");  
  lua_rawget    (L, -2);               // get getinfo function
  
  if (!lua_isfunction (L, -1))
    {
    lua_pop (L, 2);   // pop result and debug table
    lua_pushnil (L);
    return;
    }

  lua_remove (L, -2);   // remove debug table, leave traceback function
  }

int CallLuaWithTraceBack (lua_State *L, const int iArguments, const int iReturn)
  {

  int error;
  int base = lua_gettop (L) - iArguments;  /* function index */
  GetTracebackFunction (L);
  if (lua_isnil (L, -1))
    {
    lua_pop (L, 1);   // pop non-existent function
    error = lua_pcall (L, iArguments, iReturn, 0);
    }  
  else
    {
    lua_insert (L, base);  /* put it under chunk and args */
    error = lua_pcall (L, iArguments, iReturn, base);
    lua_remove (L, base);  /* remove traceback function */
    }

  return error;
  }  // end of CallLuaWithTraceBack

//-------------------------------------------------------------------------------
// stuff for popen, pclose
//-------------------------------------------------------------------------------

// see: http://lua-users.org/wiki/PipesOnWindows

/*

  This is all pretty crappy.

  I don't like the way it builds up a command (eg. cmd.exe /c blah blah )

  We need to tap into the metatable for io to handle the close correctly.

  And, it just crashes when I test it. ;)

  */


#undef POPEN_STUFF_WHICH_DOESNT_WORK

#ifdef POPEN_STUFF_WHICH_DOESNT_WORK
/*------------------------------------------------------------------------------
  Globals for the Routines pt_popen() / pt_pclose()
------------------------------------------------------------------------------*/
static HANDLE my_pipein[2], my_pipeout[2], my_pipeerr[2];
static char   my_popenmode = ' ';

static int
my_pipe(HANDLE *readwrite)
{
  SECURITY_ATTRIBUTES sa;

  sa.nLength = sizeof(sa);          /* Length in bytes */
  sa.bInheritHandle = 1;            /* the child must inherit these handles */
  sa.lpSecurityDescriptor = NULL;

  if (! CreatePipe (&readwrite[0],&readwrite[1],&sa,1 << 13))
  {
    errno = -1; /* EMFILE; que? */
    return -1;
  }

  return 0;
}

/*------------------------------------------------------------------------------
  Replacement for 'popen()' under WIN32.
  NOTE: if cmd contains '2>&1', we connect the standard error file handle
    to the standard output file handle.
------------------------------------------------------------------------------*/
FILE *
pt_popen(const char *cmd, const char *mode)
{
  FILE *fptr = (FILE *)0;
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  int success, redirect_error = 0;
  char cmd_buff[2048];
  char *err2out;

  const char *shell_cmd = getenv("COMSPEC");
  if (! shell_cmd) shell_cmd = "cmd";
  strcpy(cmd_buff,shell_cmd);
  strcat(cmd_buff," /c ");
  strcat(cmd_buff,cmd);

  my_pipein[0]   = INVALID_HANDLE_VALUE;
  my_pipein[1]   = INVALID_HANDLE_VALUE;
  my_pipeout[0]  = INVALID_HANDLE_VALUE;
  my_pipeout[1]  = INVALID_HANDLE_VALUE;
  my_pipeerr[0]  = INVALID_HANDLE_VALUE;
  my_pipeerr[1]  = INVALID_HANDLE_VALUE;

  if (!mode || !*mode)
    goto finito;

  my_popenmode = *mode;
  if (my_popenmode != 'r' && my_popenmode != 'w')
    goto finito;

  /*
   * Shall we redirect stderr to stdout ? */
  if ((err2out = strstr("2>&1",cmd)) != NULL) {
     /* this option doesn't apply to win32 shells, so we clear it out! */
     strncpy(err2out,"    ",4);
     redirect_error = 1;
  }

  /*
   * Create the Pipes... */
  if (my_pipe(my_pipein)  == -1 ||
      my_pipe(my_pipeout) == -1)
    goto finito;
  if (!redirect_error && my_pipe(my_pipeerr) == -1)
    goto finito;

  /*
   * Now create the child process */
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb           = sizeof(STARTUPINFO);
  siStartInfo.hStdInput    = my_pipein[0];
  siStartInfo.hStdOutput   = my_pipeout[1];
  if (redirect_error)
    siStartInfo.hStdError  = my_pipeout[1];
  else
    siStartInfo.hStdError  = my_pipeerr[1];
  siStartInfo.dwFlags    = STARTF_USESTDHANDLES;

  success = CreateProcess(NULL,
     (LPTSTR)cmd_buff,  // command line 
     NULL,              // process security attributes 
     NULL,              // primary thread security attributes 
     TRUE,              // handles are inherited 
     DETACHED_PROCESS,  // creation flags: without window (?)
     NULL,              // use parent's environment 
     NULL,              // use parent's current directory 
     &siStartInfo,      // STARTUPINFO pointer 
     &piProcInfo);      // receives PROCESS_INFORMATION 

  if (!success)
    goto finito;

  /*
   * These handles listen to the Child process */
  CloseHandle(my_pipein[0]);  my_pipein[0]  = INVALID_HANDLE_VALUE;
  CloseHandle(my_pipeout[1]); my_pipeout[1] = INVALID_HANDLE_VALUE;
  CloseHandle(my_pipeerr[1]); my_pipeerr[1] = INVALID_HANDLE_VALUE;

  if (my_popenmode == 'r')
    fptr = _fdopen(_open_osfhandle((long)my_pipeout[0],_O_BINARY),"r");
  else
    fptr = _fdopen(_open_osfhandle((long)my_pipein[1],_O_BINARY),"w");

finito:
  if (!fptr)
  {
    if (my_pipein[0]  != INVALID_HANDLE_VALUE)
      CloseHandle(my_pipein[0]);
    if (my_pipein[1]  != INVALID_HANDLE_VALUE)
      CloseHandle(my_pipein[1]);
    if (my_pipeout[0] != INVALID_HANDLE_VALUE)
      CloseHandle(my_pipeout[0]);
    if (my_pipeout[1] != INVALID_HANDLE_VALUE)
      CloseHandle(my_pipeout[1]);
    if (my_pipeerr[0] != INVALID_HANDLE_VALUE)
      CloseHandle(my_pipeerr[0]);
    if (my_pipeerr[1] != INVALID_HANDLE_VALUE)
      CloseHandle(my_pipeerr[1]);
  }
  return fptr;

}

/*------------------------------------------------------------------------------
  Replacement for 'pclose()' under WIN32
------------------------------------------------------------------------------*/
int
pt_pclose(FILE *fle)
{
  if (fle)
  {
    (void)fclose(fle);

    CloseHandle(my_pipeerr[0]);
    if (my_popenmode == 'r')
      CloseHandle(my_pipein[1]);
    else
     CloseHandle(my_pipeout[0]);
    return 0;
  }
  return -1;
}


// copies from Lua source

static FILE **newfile (lua_State *L) {
  FILE **pf = (FILE **)lua_newuserdata(L, sizeof(FILE *));
  *pf = NULL;  /* file handle is currently `closed' */
  luaL_getmetatable(L, LUA_FILEHANDLE);
  lua_setmetatable(L, -2);
  return pf;
}

static int pushresult (lua_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (i) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      lua_pushfstring(L, "%s", strerror(en));
    lua_pushinteger(L, en);
    return 3;
  }
}

int win_io_popen (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = pt_popen(filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}

#define topfile(L)	((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE))

int win_io_pclose (lua_State *L) {
  FILE **p = topfile(L);
  int ok = pt_pclose(*p) != -1;
  *p = NULL;
  return pushresult(L, ok, NULL);
}

#endif // POPEN_STUFF_WHICH_DOESNT_WORK

