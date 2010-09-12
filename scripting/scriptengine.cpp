#include "stdafx.h"
#include "..\mainfrm.h"
#include "..\MUSHclient.h"
#include "..\doc.h"
#include "..\dialogs\ScriptErrorDlg.h"
#include "lua_scriptengine.h"

static CString strProcedure;
static CString strType;
static CString strReason;
static bool bImmediate = true;

IScriptEngine* IScriptEngine::Create (const CString& language, CMUSHclientDoc* pDoc)
{
  if (language.CompareNoCase ("Lua") == 0)
    return new LuaScriptEngine (pDoc);
  else
    return new CScriptEngine (pDoc, language);
}

CScriptEngine::CScriptEngine (CMUSHclientDoc* pDoc, const CString strLanguage)
  : m_pDoc(pDoc), m_strLanguage(strLanguage), m_IActiveScript(NULL),
    m_IActiveScriptParse(NULL), m_site(NULL), m_pDispatch(NULL)
{
  // not under Wine
  if (bWine)
    throw ScriptEngineException ("Cannot create WSH engine under Wine.");

  try
    {
    /*
    Note: very, very important!

    To use swprintf successfully, you must specify %S for converting a single-byte
    string to a wide string, not %s. Alternatively, you can specify %hs which means
    the same thing. See reference to wprintf for details.

    If you don't, be prepared for access violations as the swprintf goes mad
    and writes all over memory, depending on whether there are two consecutive
    nulls in the string being printed.
    */

    OLECHAR wszOutput[101];
    swprintf (wszOutput, 100, OLESTR("%S"), (LPCTSTR) m_strLanguage);

    // javascript: {f414c260-6ac0-11cf-b6d1-00aa00bbbb58}
    // jscript:    {f414c260-6ac0-11cf-b6d1-00aa00bbbb58}   ???
    // python:     {DF630910-1C1D-11d0-AE36-8C0F5E000000}

    CLSID clsid;
    HRESULT hr = CLSIDFromProgID (wszOutput, &clsid);
    if (hr != S_OK)
      {
      CString strMsg;
      strMsg.Format ("finding CLSID of scripting language \"%s\"", (LPCTSTR) m_strLanguage);
      ShowError (hr, strMsg);
      throw ScriptEngineException (strMsg);
      }

    // create an instance of the VBscript/perlscript/jscript engine
    hr = ::CoCreateInstance (clsid, NULL, CLSCTX_ALL, IID_IActiveScript,
                             reinterpret_cast<void**>(&m_IActiveScript));
    if (ShowError (hr, "loading scripting engine"))
      throw ScriptEngineException ("loading scripting engine");

    hr = m_IActiveScript->QueryInterface (IID_IActiveScriptParse, 
                                          reinterpret_cast<void**>(&m_IActiveScriptParse));
    if (ShowError (hr, "locating parse interface"))
      throw ScriptEngineException ("locating parse interface");

    hr = m_IActiveScriptParse->InitNew ();
    if (ShowError (hr, "initialising scripting engine"))
      throw ScriptEngineException ("initialising scripting engine");

    // create host site object
    m_site = new CActiveScriptSite (m_pDoc->GetIDispatch (TRUE), m_pDoc);
    m_site->AddRef ();

    hr = m_IActiveScript->SetScriptSite (m_site);
    if (ShowError (hr, "setting site"))
      throw ScriptEngineException ("setting site");

    // add world object to engine's namespace
    // added SCRIPTITEM_GLOBALMEMBERS in 3.27

    WCHAR charWorld[] = L"world";

    hr = m_IActiveScript->AddNamedItem (
        charWorld,
        SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE | SCRIPTITEM_GLOBALMEMBERS);
    if (ShowError (hr, "adding world to script engine"))
      throw ScriptEngineException ("adding world to script engine");

    // set state to started
    hr = m_IActiveScript->SetScriptState (SCRIPTSTATE_STARTED);
    if (ShowError (hr, "starting script engine"))
      throw ScriptEngineException ("starting script engine");

    // connect outbound objects (ie. world)
    hr = m_IActiveScript->SetScriptState (SCRIPTSTATE_CONNECTED);
    if (ShowError (hr, "connecting script engine"))
      throw ScriptEngineException ("connecting script engine");

    // get script engine dispatch pointer
    hr = m_IActiveScript->GetScriptDispatch (0, &m_pDispatch);
    if (ShowError (hr, "getting script engine dispatch pointer"))
      throw ScriptEngineException ("getting script engine dispatch pointer");
    }
  catch (HRESULT hr)
    {
    ShowError (hr, "starting scripting support");
    throw ScriptEngineException ("starting scripting support");
    }
  catch (ScriptEngineException& ex)
    {
    ShowError (true, ex.what ());
    throw;
    }
  catch (...)
    {
    ::TMessageBox ("Something nasty happened whilst initialising the scripting engine");
    throw;
    }
}

CScriptEngine::~CScriptEngine ()
{
  // release engine
  if (m_IActiveScript)
    {
    m_IActiveScript->SetScriptState(SCRIPTSTATE_DISCONNECTED);
    m_IActiveScript->Close ();
    m_IActiveScript->Release ();
    }

  // release parser
  if (m_IActiveScriptParse)
    m_IActiveScriptParse->Release ();

  // release site
  if (m_site)
    m_site->Release ();
}

// returns true if error
bool CScriptEngine::Execute (DISPID & dispid,  // dispatch ID, will be set to DISPID_UNKNOWN on an error
                             LPCTSTR szProcedure,  // eg. ON_TRIGGER_XYZ
                             const unsigned short iReason,  // value for m_iCurrentActionSource
                             LPCTSTR szType,   // eg. trigger, alias
                             LPCTSTR szReason, // eg. trigger subroutine XXX
                             DISPPARAMS & params,  // parameters
                             long & nInvocationCount,  // count of invocations
                             COleVariant * result)   // result of call
{
  // don't do it if no routine address 
  if (dispid == DISPID_UNKNOWN)
    return false;

  return this->Invoke (dispid, params, result,
                       szProcedure, szType, szReason, iReason,
                       nInvocationCount);
}
 
bool CScriptEngine::Invoke (DISPID& dispid,
                            DISPPARAMS& params,
                            COleVariant* result,
                            LPCTSTR procedure,
                            LPCTSTR type,
                            LPCTSTR reason,
                            const unsigned short reason_code,
                            long& invocation_count)
{
  strProcedure = procedure;
  strType = type;
  strReason = reason;
  bImmediate = false;

  unsigned short iOldStyle = m_pDoc->m_iNoteStyle;
  m_pDoc->m_iNoteStyle = NORMAL;    // back to default style

  HRESULT hr;
  try
    {
    m_pDoc->Trace (TFormat ("Executing %s script \"%s\"", type, procedure));

    SCRIPTSTATE ss;
    hr = m_IActiveScript->GetScriptState (&ss);
    if (hr == S_OK && ss != SCRIPTSTATE_CONNECTED)
      hr = m_IActiveScript->SetScriptState (SCRIPTSTATE_CONNECTED);

    if (hr != S_OK)
      {
      throw ScriptEngineException(TFormat (
          "Script engine problem invoking subroutine \"%s\" when %s",
          procedure, reason));
      }

    LARGE_INTEGER start;
    if (App.m_iCounterFrequency)
      QueryPerformanceCounter (&start);

    if (reason_code != eDontChangeAction)
      m_pDoc->m_iCurrentActionSource = reason_code;

    // Invoke the script
    hr = m_pDispatch->Invoke (dispid, IID_NULL, 0, DISPATCH_METHOD,
        &params, result, NULL, NULL);

    if (reason_code != eDontChangeAction)
      m_pDoc->m_iCurrentActionSource = eUnknownActionSource;

    if (hr == S_OK && App.m_iCounterFrequency)
      {
      LARGE_INTEGER finish;
      QueryPerformanceCounter (&finish);
      m_pDoc->m_iScriptTimeTaken += finish.QuadPart - start.QuadPart;
      }

    if (hr != S_OK)
      {
      dispid = DISPID_UNKNOWN; // stop further invocations

      if (hr == 0x800a01c2) // wrong number of arguments
        throw ScriptEngineException (TFormat (
              "Wrong number of arguments for script subroutine \"%s\" when %s"
              "\n\nWe expected your subroutine to have %i argument%s",
            procedure, reason, PLURAL (params.cArgs)));
      else
        throw ScriptEngineException (TFormat (
            "Unable to invoke script subroutine \"%s\" when %s",
            procedure, reason));
      }
    }
  catch (ScriptEngineException& ex)
    {
    strProcedure.Empty ();
    strType.Empty ();
    strReason.Empty ();
    bImmediate = true;

    m_pDoc->m_iNoteStyle = iOldStyle;

    ::UMessageBox (ex.what ());
    return true;
    }
  catch (...)
    {
    strProcedure.Empty ();
    strType.Empty ();
    strReason.Empty ();
    bImmediate = true;

    m_pDoc->m_iNoteStyle = iOldStyle;
    throw;
    }

  // count number of times used
  invocation_count += 1;

  // cleanups
  strProcedure.Empty ();
  strType.Empty ();
  strReason.Empty ();
  bImmediate = true;

  m_pDoc->m_iNoteStyle = iOldStyle;

  // true on error
  return hr != S_OK;
}


STDMETHODIMP CActiveScriptSite::OnScriptError(IActiveScriptError *pscripterror) 
  {
  DWORD dwCookie;
  LONG nChar;
  ULONG nLine;
  BSTR bstr = 0;
  EXCEPINFO ei; 
  ZeroMemory(&ei, sizeof(ei));

  TRACE ("CActiveScriptSite: OnScriptError\n");
  
  pscripterror->GetSourcePosition(&dwCookie, &nLine, &nChar);
  pscripterror->GetSourceLineText(&bstr);
  pscripterror->GetExceptionInfo(&ei);
    
  CScriptErrorDlg dlg;

  if (ei.wCode)
    dlg.m_iError = ei.wCode;
  else
    dlg.m_iError = ei.scode;

  nLine = nLine + 1;  // make 1-relative to be consistent with lua

  dlg.m_strEvent = TFormat ("Execution of line %i column %i",
                                  nLine, nChar + 1);

  dlg.m_strDescription = ei.bstrDescription;

  if (!strProcedure.IsEmpty ())
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
    dlg.m_strCalledBy = Translate ("Immediate execution");

    dlg.m_strDescription += ENDLINE;
    if (bstr)
      {
      dlg.m_strDescription += Translate ("Line in error: ");
      dlg.m_strDescription += ENDLINE;
      dlg.m_strDescription += bstr;
      }
    }

  dlg.m_strRaisedBy = Translate ("No active world");

  if (m_pDoc)
    {
    if (m_pDoc->m_CurrentPlugin)
      {
      dlg.m_strRaisedBy = TFormat ("Plugin: %s (called from world: %s)",
                          (LPCTSTR) m_pDoc->m_CurrentPlugin->m_strName,
                          (LPCTSTR) m_pDoc->m_mush_name);
      }
    else 
      dlg.m_strRaisedBy = TFormat ("World: %s", (LPCTSTR) m_pDoc->m_mush_name);
    }

  if (!m_pDoc || !m_pDoc->m_bScriptErrorsToOutputWindow)
    {
    if (m_pDoc)
      dlg.m_bHaveDoc = true;
    dlg.DoModal ();
    if (m_pDoc && dlg.m_bUseOutputWindow)
      {
      m_pDoc->m_bScriptErrorsToOutputWindow = true;
      m_pDoc->SetModifiedFlag (TRUE);
      }
    }
  else
    {
    m_pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, Translate ("Script error"));
    m_pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strRaisedBy);
    m_pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strEvent);
    m_pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strCalledBy);
    m_pDoc->ColourNote (SCRIPTERRORFORECOLOUR, SCRIPTERRORBACKCOLOUR, dlg.m_strDescription);

// show bad lines?

    if (!bImmediate)
      m_pDoc->ShowErrorLines (nLine);

   
    }  // end of showing in output window
  
  SysFreeString(bstr);
  SysFreeString(ei.bstrSource);
  SysFreeString(ei.bstrDescription);
  SysFreeString(ei.bstrHelpFile);
  
  return S_OK;
  }   // end of CActiveScriptSite::OnScriptError

bool CScriptEngine::Parse (const CString & strCode, const CString & strWhat)
{
  bImmediate = (strWhat != "Script file"  && strWhat != "Plugin");

  if (!m_IActiveScriptParse || !m_IActiveScript)
    return true;   // no script engine

  // new for Python - an error may have caused the script state to change
  // if so, try to put it back to connected
  SCRIPTSTATE ss;
  HRESULT hr = m_IActiveScript->GetScriptState (&ss);
  if (hr == S_OK && ss != SCRIPTSTATE_CONNECTED)
    hr = m_IActiveScript->SetScriptState (SCRIPTSTATE_CONNECTED);

  if (hr != S_OK)
    {
    ::TMessageBox ("Script engine problem on script parse");
    return true;
    }

  BSTR bstrCode = strCode.AllocSysString ();
  if (!bstrCode)
    return true;  // error

  EXCEPINFO ei;
  ZeroMemory(&ei, sizeof(ei));

  LARGE_INTEGER start, finish;
  if (App.m_iCounterFrequency)
    QueryPerformanceCounter (&start);

  hr = m_IActiveScriptParse->ParseScriptText
                               (bstrCode, 0, 0, 0, 0, 0, 
                               SCRIPTTEXT_ISPERSISTENT |
                                SCRIPTTEXT_ISVISIBLE,
                               NULL, 
                               &ei);

  if (hr == S_OK && App.m_iCounterFrequency)
    {
    QueryPerformanceCounter (&finish);
    m_pDoc->m_iScriptTimeTaken += finish.QuadPart - start.QuadPart;
    }

  ::SysFreeString (bstrCode);

  return hr != S_OK; // true = error
} // end of CScriptEngine::Parse

DISPID CScriptEngine::GetDispid (const CString & strName)
{
  if (!m_pDispatch)
    return DISPID_UNKNOWN;   // no script engine

  DISPID dispid;

  BSTR bstrProcedure = strName.AllocSysString ();
  HRESULT hr = m_pDispatch->GetIDsOfNames (IID_NULL, &bstrProcedure, 1, LOCALE_SYSTEM_DEFAULT, &dispid);    // was LCID of 9
  ::SysFreeString (bstrProcedure);

  return (hr == S_OK) ? dispid : DISPID_UNKNOWN;
  // might be zero, the way PHP currently is :)
} // end of CScriptEngine::GetDispid

bool CScriptEngine::ShowError (const HRESULT hr, const CString& strMsg)
{
  static const char * sForeColour = "darkorange";
  static const char * sBackColour = "black";

  if (hr == S_OK)
    return false;

  DWORD status = 0;
  char *formattedmsg;

  CScriptErrorDlg dlg;
  dlg.m_iError = hr;
  dlg.m_strEvent = strMsg;
  
  if (!FormatMessage (
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, 
      hr, 
      LANG_NEUTRAL, 
      (LPTSTR) &formattedmsg, 
      0, 
      NULL))
    {
    status = GetLastError ();
    dlg.m_strDescription.Format ("<<Unable to convert error number %ld>>", hr);
    }
  else
    {
    dlg.m_strDescription.Format ("Error %ld occurred when %s:\n\n%s", 
                                 hr,
                                 (LPCTSTR) strMsg,
                                 formattedmsg);
    LocalFree (formattedmsg);
    }

  dlg.m_strRaisedBy = "No active world";

  if (m_pDoc)
    {
    if (m_pDoc->m_CurrentPlugin)
      {
      dlg.m_strRaisedBy = "Plugin: " +  m_pDoc->m_CurrentPlugin->m_strName;
      dlg.m_strRaisedBy += " (called from world: " + m_pDoc->m_mush_name + ")";
      }
    else 
      dlg.m_strRaisedBy = "World: " + m_pDoc->m_mush_name;
    }

  if (!m_pDoc || !m_pDoc->m_bScriptErrorsToOutputWindow)
    {
    if (m_pDoc)
      dlg.m_bHaveDoc = true;
    dlg.DoModal ();
    if (m_pDoc && dlg.m_bUseOutputWindow)
      {
      m_pDoc->m_bScriptErrorsToOutputWindow = true;
      m_pDoc->SetModifiedFlag (TRUE);
      }
    }
  else
    {
    m_pDoc->ColourNote (sForeColour, sBackColour, strMsg);
    m_pDoc->ColourNote (sForeColour, sBackColour, dlg.m_strRaisedBy);
//    m_pDoc->ColourNote (sForeColour, sBackColour, dlg.m_strCalledBy);
    m_pDoc->ColourNote (sForeColour, sBackColour, dlg.m_strDescription);
    }

  return true;
} // end of CScriptEngine::ShowError
