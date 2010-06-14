// types used by the world document

#pragma once

// wildcard definitions for triggers
enum {
  DEFAULT_TRIGGER_SEQUENCE = 100,
  DEFAULT_ALIAS_SEQUENCE   = 100,
};

/*

Trigger iMatch flag will be as follows:

  BFS    

  B = background colour, see enum in doc.h
  F = foreground colour, see enum in doc.h
  S = style bits, see define in doc.h

*/

enum {
  TRIGGER_MATCH_TEXT      = 0x0080, // high order of text nibble
  TRIGGER_MATCH_BACK      = 0x0800, // high order of back nibble
  TRIGGER_MATCH_HILITE    = 0x1000,
  TRIGGER_MATCH_UNDERLINE = 0x2000,
  TRIGGER_MATCH_BLINK     = 0x4000,
  TRIGGER_MATCH_INVERSE   = 0x8000,
};

// where triggers/aliases/timers get sent

enum {
  eSendToWorld,        //  0   - send to MUD
  eSendToCommand,      //  1   - put in command window
  eSendToOutput,       //  2   - display in output window
  eSendToStatus,       //  3   - put in status line
  eSendToNotepad,      //  4   - new notepad
  eAppendToNotepad,    //  5   - append to notepad
  eSendToLogFile,      //  6   - put in log file
  eReplaceNotepad,     //  7   - replace notepad
  eSendToCommandQueue, //  8   - queue it
  eSendToVariable,     //  9   - set a variable
  eSendToExecute,      // 10   - re-parse as command
  eSendToSpeedwalk,    // 11   - send to MUD as speedwalk
  eSendToScript,       // 12   - send to script engine
  eSendImmediate,      // 13   - send without queuing
  eSendToScriptAfterOmit, // 14   - send to script, after omitting from output
#ifdef PANE
       eSendToPane,         // 15   - send to a pane window
#endif
//  eSendDelayed,        // ??
  eSendToLast, // THIS MUST BE LAST!!!
};

// File types
enum { WORLD, TRIGGER, ALIAS, COLOUR, MACRO, STRING, TIMER };


// Alias match types
enum { START_OF_LINE, ANYWHERE, EXACT };

// Macro send types
enum { REPLACE_COMMAND, SEND_NOW, ADD_TO_COMMAND };

// String types
enum {
  STR_PAGE_INCOMING_1,
  STR_PAGE_INCOMING_2,
  STR_PAGE_OUTGOING_1,
  STR_PAGE_OUTGOING_2,
  STR_WHISPER_INCOMING_1,
  STR_WHISPER_INCOMING_2,
  STR_WHISPER_OUTGOING_1,
  STR_WHISPER_OUTGOING_2,
  STR_MAIL,
  STR_GAME,
  STR_SET,
}; // end of string types


// macro types
enum {
  MAC_UP,
  MAC_DOWN,
  MAC_NORTH,
  MAC_SOUTH,
  MAC_EAST,
  MAC_WEST,
  MAC_EXAMINE,
  MAC_LOOK,
  MAC_PAGE,
  MAC_SAY,
  MAC_WHISPER,
  MAC_DOING,
  MAC_WHO,
  MAC_DROP,
  MAC_TAKE,
  MAC_F2,
  MAC_F3,
  MAC_F4,
  MAC_F5,
  MAC_F7,
  MAC_F8,
  MAC_F9,
  MAC_F10,
  MAC_F11,
  MAC_F12,
  MAC_SHIFT_F2,
  MAC_SHIFT_F3,
  MAC_SHIFT_F4,
  MAC_SHIFT_F5,
  MAC_SHIFT_F6,
  MAC_SHIFT_F7,
  MAC_SHIFT_F8,
  MAC_SHIFT_F9,
  MAC_SHIFT_F10,
  MAC_SHIFT_F11,
  MAC_SHIFT_F12,
  MAC_CTRL_F2,
  MAC_CTRL_F3,
  MAC_CTRL_F5,
  MAC_CTRL_F7,
  MAC_CTRL_F8,
  MAC_CTRL_F9,
  MAC_CTRL_F10,
  MAC_CTRL_F11,
  MAC_CTRL_F12,
  MAC_LOGOUT,
  MAC_QUIT,
    // new ones - v 3.42
  MAC_ALT_A,
  MAC_ALT_B,
  MAC_ALT_J,
  MAC_ALT_K,
  MAC_ALT_L,
  MAC_ALT_M,
  MAC_ALT_N,
  MAC_ALT_O,
  MAC_ALT_P,
  MAC_ALT_Q,
  MAC_ALT_R,
  MAC_ALT_S,
  MAC_ALT_T,
  MAC_ALT_U,
  MAC_ALT_X,
  MAC_ALT_Y,
  MAC_ALT_Z,
    // new ones - v 3.44
  MAC_F1,
  MAC_CTRL_F1,
  MAC_SHIFT_F1,
  MAC_F6,
  MAC_CTRL_F6,

  MACRO_COUNT, // this must be last!! 
};  // end of macro types

/////////////////////////////////////////////////////////////////////////////
//  CAction - these are actions that are done by a hyperlink

/*

  To save memory we will share these actions between links, and just store
  a pointer to it in the style run.

  Some of them can get fairly long, eg:

  Action - send to MUD: "Wear #24850|look #24850|examine #24850|
                         drop #24850~Wear The Curse of Kral|
                         Look at The Curse of Kral|Examine The Curse of Kral|
                         Drop The Curse of Kral"

  ... and then the above may be repeated for every *character* of the hyperlink
      if they are in different colours.

  Now, assuming that you go to the same rooms more than once, or look at your
  inventory more than once, such items will be cached, by being stored once in the
  CAction list. I use a reference count to know if we can ever get rid of one.

  */

class CAction : public CObject
{
  public:
    CAction (const CString & strAction,
             const CString & strHint,
             const CString & strVariable,
             CMUSHclientDoc * pDoc);

    CString m_strAction;   // What to send - multiple ones delimited by |
    CString m_strHint;     // Hint - flyover hint, and prompts for actions
    CString m_strVariable; // which variable to set (FLAG in MXP)
    unsigned long m_iHash; // for quick lookups - hash of action, hint, variable

    // call AddRef when another style uses this action
    void AddRef ();
    // call Release when a style no longer uses the action
    void Release ();

    int GetReferenceCount ()
    { return m_iRefCount; }

  protected: 
    int m_iRefCount;         // number of users of this item
    CMUSHclientDoc * m_pDoc; // which document it belongs to
};

typedef CTypedPtrList <CPtrList, CAction*> CActionList;

/*
  25 May 2001.

  In order to accomodate MXP, and future expansion such as multiple fonts,
  hyperlinks, RGB colours, and so on, I am moving from a straight "style"
  array (at at pre 3.10) to a linked list of styles.

  Each item in the list will describe text that is to be drawn in an identical
  way, and have the same effect (eg. the same hyperlink).

  The number of bytes affected (in "text" in CLine) will be in iLength.

  */

/////////////////////////////////////////////////////////////////////////////
//  CStyle


// values for COLOURTYPE
enum {
  COLOUR_ANSI     = 0x0000, //  ANSI colour from ANSI table
  COLOUR_CUSTOM   = 0x0100, //  Custom colour from custom table
  COLOUR_RGB      = 0x0200, //  RGB colour in iColour
  COLOUR_RESERVED = 0x0300, //  reserved
};

// values for ACTIONTYPE
enum {
  ACTION_NONE      = 0x0000, // do nothing
  ACTION_SEND      = 0x0400, // send strAction to MUD
  ACTION_HYPERLINK = 0x0800, // http:// blah or mailto:blah
  ACTION_PROMPT    = 0x0C00, // send strAction to command window
};

// style flags 

#define NORMAL     0x0000   // a mnemonic way of clearing all attributes
#define HILITE     0x0001   // bold
#define UNDERLINE  0x0002   // underline
#define BLINK      0x0004   // italic??
#define INVERSE    0x0008   // need to invert it
#define CHANGED    0x0010   // colour has been changed by a trigger
#define COLOURTYPE 0x0300   // type of colour in iForeColour/iBackColour, see above
#define ACTIONTYPE 0x0C00   // action type, see above
#define STYLE_BITS 0x0FFF   // everything except START_TAG
#define START_TAG  0x1000   // strAction is tag name - start of tag (eg. <b> )
#define TEXT_STYLE 0x000F   // bold, underline, italic, inverse flags

// eg. <send "command1|command2|command3" hint="click to see menu|Item 1|Item 2|Item 2">this is a menu link</SEND>

#define POPUP_DELIMITER "|"  // delimiter between different popup menu items

class CStyle : public CObject
{
  public:
    unsigned short iLength;     // how many bytes (letters) are affected in "text"
    unsigned short iFlags;      // see define above
    COLORREF       iForeColour; // RGB foreground colour, or ANSI/custom colour number
    COLORREF       iBackColour; // RGB background colour, or ANSI/custom colour number
    CAction *      pAction;     // what action, if any this item carries out
                                //  - also stores variables  
    CStyle ()
      : iForeColour(WHITE), iBackColour(BLACK),
        iLength(0), iFlags(0),
        pAction(NULL)
    {}

    ~CStyle () 
    {
      if (pAction)
        pAction->Release ();
    }
};

typedef CTypedPtrList <CPtrList, CStyle*> CStyleList;


/////////////////////////////////////////////////////////////////////////////
//  CLine

// bit settings for "flags" below
enum {
  COMMENT         = 0x01, // this is a comment from a script
  USER_INPUT      = 0x02, // this is echoed user input
  LOG_LINE        = 0x04, // this line is to be logged
  BOOKMARK        = 0x08, // line is bookmarked
  HORIZ_RULE      = 0x10, // line is a horizontal rule
  NOTE_OR_COMMAND = 0x03, // for testing if line is an output line or not
};

class CLine : public CObject
{
  public:
    bool hard_return;
    unsigned char flags;
    int len;
    int last_space;
    char * text;          // allocated as necessary and then resized
    CStyleList styleList; // list of styles applying to text, see above
    CTime m_theTime;      // time this line arrived
    LARGE_INTEGER m_lineHighPerformanceTime;  
    int iMemoryAllocated; // size of buffer allocated for "text"

    long m_nLineNumber;

    CLine (const long nLineNumber, 
           const unsigned int nWrapColumn,
           const unsigned short iFlags,      
           const COLORREF iForeColour,
           const COLORREF iBackColour,
           const bool bUnicode 
           );

    ~CLine ();
};

typedef CTypedPtrList <CPtrList, CLine*> CLineList;

/////////////////////////////////////////////////////////////////////////////
//  CAlias

class CAlias : public CObject
{
  DECLARE_DYNAMIC(CAlias)

  public:
    CAlias ()
      : bIgnoreCase(FALSE), bEnabled(TRUE), dispid(DISPID_UNKNOWN),
        nUpdateNumber(0), nInvocationCount(0), nMatched(0),
        bExpandVariables(FALSE), bOmitFromLog(FALSE), bOmitFromOutput(FALSE),
        bRegexp(FALSE), regexp(NULL), iSequence(DEFAULT_ALIAS_SEQUENCE),
        bKeepEvaluating(FALSE), bMenu(FALSE), tWhenMatched(0),
        bTemporary(false), bIncluded(false), bSelected(false),
        iSendTo(eSendToWorld), bEchoAlias(false), bOneShot(false),
        bOmitFromCommandHistory(false), iUserOption(0), bExecutingScript(false)
    {
      wildcards.resize (MAX_WILDCARDS);
    }

    ~CAlias ()
    { delete regexp; }

    CString name;
    CString contents;
    unsigned short bIgnoreCase;  

    // new in version 8

    CString strLabel;                // alias label
    CString strProcedure;            // procedure to execute
    unsigned short bEnabled;         // alias enabled
    unsigned short bExpandVariables; // expand variables (eg. @food)
    unsigned short bOmitFromLog;     // omit from log file?
    unsigned short bRegexp;          // use regular expressions
    unsigned short bOmitFromOutput;  // omit alias from output screen?

    // new in version 12

    unsigned short iSequence;  // which order, lower is sooner
  //  unsigned short bSpeedWalk; // true means evaluate as speed walk
  //  unsigned short bQueue;   // true means queue it rather than send it
    unsigned short bMenu;      // make pop-up menu from this alias?
  //  unsigned short bDelayed;   // true means use AddTimer, eg. 5; go east

    // in XML version

    CString strGroup;                       // group it belongs to
    CString strVariable;                    // which variable to set (for send to variable)
  //  unsigned short bSetVariable;          // if true, set variable rather than send it
    unsigned short iSendTo;                 // where alias is sent to (see enum above)
    unsigned short bKeepEvaluating;         // if true, keep evaluating triggers
    unsigned short bEchoAlias;              // if true, echo alias itself to the output window
    long iUserOption;                       // user-settable flags etc.
    unsigned short bOmitFromCommandHistory; // omit from command history
    BOOL bOneShot;                          // if true, alias only fires once

    // computed at run-time

    DISPID dispid;            // dispatch ID for calling the script
    __int64 nUpdateNumber;    // for detecting update clashes
    long  nInvocationCount;   // how many times procedure called
    long  nMatched;           // how many times the alias matched
    vector<string> wildcards; // matching wildcards
    t_regexp * regexp;        // compiled regular expression, if needed
    CTime tWhenMatched;       // when last matched
    bool bTemporary;          // if true, don't save it
    bool bIncluded;           // if true, don't save it
    bool bSelected;           // if true, selected for use in a plugin
    bool bExecutingScript;    // if true, executing a script and cannot be deleted
    CString strInternalName;  // name it is stored in the alias map under
};

// map for lookup by name
typedef CTypedPtrMap <CMapStringToPtr, CString, CAlias*> CAliasMap;
// array for saving in order
typedef CTypedPtrArray <CPtrArray, CAlias*> CAliasArray;
// list for alias evaluation
typedef CTypedPtrList <CPtrList, CAlias*> CAliasList;


/////////////////////////////////////////////////////////////////////////////
//  CTrigger

enum {
  TRIGGER_COLOUR_CHANGE_BOTH = 0x00,
  TRIGGER_COLOUR_CHANGE_FOREGROUND = 0x01,
  TRIGGER_COLOUR_CHANGE_BACKGROUND = 0x02,
};


class CTrigger : public CObject
{
  DECLARE_DYNAMIC(CTrigger)

  public:
    CTrigger ()
      : ignore_case(FALSE), omit_from_log(false), bOmitFromOutput(FALSE),
        bKeepEvaluating(FALSE), bExpandVariables(FALSE), iSendTo(eSendToWorld),
        bEnabled(TRUE), dispid(DISPID_UNKNOWN), nUpdateNumber(0), colour(0),
        nInvocationCount(0), iClipboardArg(0), nMatched(0), bRegexp(false),
        bRepeat(false), regexp(NULL), iSequence(DEFAULT_TRIGGER_SEQUENCE),
        iMatch(0), iStyle(0), bSoundIfInactive(false), tWhenMatched(0),
        bLowercaseWildcard(false), bTemporary(false), bIncluded(false),
        bSelected(false), iUserOption(0), iOtherForeground(0),
        iOtherBackground(0), bMultiLine(false), iLinesToMatch(0),
        iColourChangeType(TRIGGER_COLOUR_CHANGE_BOTH),
        bExecutingScript(false), bOneShot(FALSE)
    {
      wildcards.resize (MAX_WILDCARDS);
    }

    ~CTrigger () { delete regexp; };

    CString trigger;
    CString contents;
    CString sound_to_play;
    unsigned short ignore_case;    // if true, not case-sensitive
    unsigned short colour;         // (user) colour to display in
    unsigned short  omit_from_log; // if true, do not log triggered line

    // new in version 7

    unsigned short bOmitFromOutput; // if true, do not put triggered line in output
    unsigned short bKeepEvaluating; // if true, keep evaluating triggers
    unsigned short bEnabled;        // if true, this trigger is enabled
    CString strLabel;               // trigger label

    // new in version 8

    CString strProcedure;         // procedure to execute
    unsigned short iClipboardArg; // if non-zero, copy matching wildcard to clipboard

    // new in version 10

    unsigned short iSendTo;  // where trigger is sent to (see enum above)
    unsigned short  bRegexp; // use regular expressions

    // new in version 11

    unsigned short  bRepeat; // repeat on same line until no more matches

    // new in version 12

    unsigned short iSequence;        // which order, lower is sooner
    unsigned short iMatch;           // match on colour/bold/italic? see define at start of file
    unsigned short iStyle;           // underline, italic, bold
    unsigned short bSoundIfInactive; // only play sound if window inactive
    unsigned short bExpandVariables; // expand variables in the trigger

    // new in XML

    bool bLowercaseWildcard;          // convert wildcards to lower case (for %1 etc.)
    CString strGroup;                 // group it belongs to
    CString strVariable;              // which variable to set (for send to variable)
    long iUserOption;                 // user-settable flags etc.
    COLORREF iOtherForeground;        // "other" foreground colour
    COLORREF iOtherBackground;        // "other" background colour
    unsigned short bMultiLine;        // do we do a multi-line match?
    unsigned short iLinesToMatch;     // if so, on how many lines?
    unsigned short iColourChangeType; // does a colour change affect text (1), background (2) or both (0)?
                                      // see define just above?
    BOOL bOneShot;                    // if true, trigger only fires once
#ifdef PANE
    CString strPane;    // which pane to send to (for send to pane)
#endif

    // computed at run-time

    DISPID dispid;            // dispatch ID for calling the script
    __int64 nUpdateNumber;    // for detecting update clashes
    long  nInvocationCount;   // how many times procedure called
    long  nMatched;           // how many times the trigger fired
    vector<string> wildcards; // matching wildcards
    t_regexp * regexp;        // compiled regular expression, if needed
    CTime tWhenMatched;       // when last matched
    bool bTemporary;          // if true, don't save it
    bool bIncluded;           // if true, don't save it
    bool bSelected;           // if true, selected for use in a plugin
    bool bExecutingScript;    // if true, executing a script and cannot be deleted
    CString strInternalName;  // name it is stored in the trigger map under
};

// map for lookup by name
typedef CTypedPtrMap <CMapStringToPtr, CString, CTrigger*> CTriggerMap;
// array for sequencing evaluation
typedef CTypedPtrArray <CPtrArray, CTrigger*> CTriggerArray;
// list for trigger evaluation
typedef CTypedPtrList <CPtrList, CTrigger*> CTriggerList;

/////////////////////////////////////////////////////////////////////////////
//  CTimer

class CTimer : public CObject
{
  DECLARE_DYNAMIC(CTimer)

  public:
    enum {
      eInterval = 0,
      eAtTime,
    };

    CTimer ()
      : nInvocationCount(0), nMatched(0), iType(eInterval),
        iAtHour(0), iAtMinute(0), fAtSecond(0.0f),
        iEveryHour(0), iEveryMinute(0), fEverySecond(0.0f),
        iOffsetHour(0), iOffsetMinute(0), fOffsetSecond(0.0f),
        bOneShot(FALSE), bActiveWhenClosed(FALSE),
        bTemporary(false), bIncluded(false), bSelected(false),
        bOmitFromOutput(false), bOmitFromLog(false), bExecutingScript(false),
        dispid(DISPID_UNKNOWN), tFireTime(CmcDateTime::GetTimeNow()),
        iSendTo(eSendToWorld), iUserOption(0)
    {}

    int iType;           // at or interval, see enum above
    CString strContents; // what to send when it triggers

    // for daily timers
    int iAtHour;
    int iAtMinute;
    double fAtSecond;

    // for periodical times
    int iEveryHour;
    int iEveryMinute;
    double fEverySecond;

    // offset for periodical times
    int iOffsetHour;
    int iOffsetMinute;
    double fOffsetSecond;

    BOOL bEnabled;          // if true, this timer is enabled
    CString strLabel;       // timer label
    CString strProcedure;   // procedure to execute
    BOOL bOneShot;          // if true, timer only fires once
  //  BOOL bSpeedWalk;        // do speed walk when timer fires
  //  BOOL bNote;             // do world.note when timer fires
    BOOL bActiveWhenClosed; // fire when world closed

    // in XML version
    CString strGroup;               // group it belongs to
    unsigned short iSendTo;         // where timer is sent to (see enum above)
    CString strVariable;            // which variable to set
    long iUserOption;               // user-settable flags etc.
    unsigned short bOmitFromOutput; // omit timer from output screen?
    unsigned short bOmitFromLog;    // omit timer line from log file?

    // computed at run-time
    DISPID dispid;         // dispatch ID for calling the script
    __int64 nUpdateNumber; // for detecting update clashes
    long nInvocationCount; // how many times procedure called
    long nMatched;         // how many times the timer fired

    // calculated field - when timer is next to go off (fire)
    CmcDateTime tFireTime; // when to fire it

    CmcDateTime tWhenFired; // when last reset/fired
    bool bTemporary;        // if true, don't save it
    bool bIncluded;         // if true, don't save it
    bool bSelected;         // if true, selected for use in a plugin
    bool bExecutingScript;  // if true, executing a script and cannot be deleted
};

typedef CTypedPtrMap <CMapStringToPtr, CString, CTimer*> CTimerMap;

/////////////////////////////////////////////////////////////////////////////
//  CVariable

class CVariable : public CObject
{
  DECLARE_DYNAMIC(CVariable)

  public:
    CVariable ()
      : bSelected(false)
    {}

    CString strLabel;
    CString strContents;

    // computed at run-time
    __int64 nUpdateNumber; // for detecting update clashes
    bool bSelected;        // if true, selected for use in a plugin
};

typedef CTypedPtrMap <CMapStringToPtr, CString, CVariable*> CVariableMap;
typedef CTypedPtrArray <CPtrArray, CVariable*> CVariableArray;


/////////////////////////////////////////////////////////////////////////////
//  CArgument - these are arguments to an MXP tag

class CArgument : public CObject
{
  public:
    CArgument (const CString& strSourceName,
               const CString& strSourceValue,
               const int& iSourcePosition)
    : strName(strSourceName), strValue(strSourceValue),
      iPosition(iSourcePosition), bKeyword(false), bUsed(false)
    {}

    CArgument ()
      : iPosition(0), bKeyword(false), bUsed(false)
    {}

    CString strName;   // name of argument, eg. color
    CString strValue;  // value of argument, eg. red
    int     iPosition; // where in argument list it is (first is 1)
    bool    bKeyword;  // true if a keyword (eg. OPEN, EMPTY)
    bool    bUsed;     // true if used
};

typedef CTypedPtrList <CPtrList, CArgument*> CArgumentList;


/////////////////////////////////////////////////////////////////////////////
//  CAtomicElement - these are atomic MXP tags that we recognise, eg. <b> 

class CAtomicElement : public CObject
{
  public:
    CString strName;  // tag name, eg. "bold"
    CString strArgs;  // supported arguments, eg. href,hint
    int     iFlags;   // see defines in mxp.h - secure, command flags
    int     iAction;  // its action, eg. MXP_ACTION_BOLD
};

typedef CTypedPtrMap <CMapStringToPtr, CString, CAtomicElement*> CAtomicElementMap;


/////////////////////////////////////////////////////////////////////////////
//  CElementItem - these are arguments to an MXP tag

class CElementItem : public CObject
{
  public:
    CElementItem () 
      : pAtomicElement(NULL)
    {}

    ~CElementItem () 
    { DELETE_LIST (ArgumentList); }

    CAtomicElement * pAtomicElement;  // pointer to appropriate atomic element
    CArgumentList  ArgumentList;      // list of arguments to this element item
};

typedef vector<CElementItem*> CElementItemList;
typedef vector<CElementItem*>::iterator ElementItemsIterator;


/////////////////////////////////////////////////////////////////////////////
//  CElement - these are user-defined MXP tags that we recognise, eg. <boldcolour>
//  eg. <!ELEMENT boldtext '<COLOR &col;><B>' ATT='col=red'>

class CElement : public CObject
{
  public:
    CElement ()
      : iTag(0), bOpen(false), bCommand(false)
    {}

    ~CElement () 
    {
      for (ElementItemsIterator itr = ElementItemList.begin(); itr != ElementItemList.end(); ++itr)
        delete *itr;
      ElementItemList.clear();

      DELETE_LIST (AttributeList);
    }

    CString strName;                  // tag name
    CElementItemList ElementItemList; // what atomic elements it defines  (arg 1)
    CArgumentList AttributeList;      // list of attributes to this element (ATT="xx")
    int iTag;                         // line tag number (20 - 99)  (TAG=n)
    CString strFlag;                  // which variable to set      (SET x)
    bool bOpen;                       // true if element is open    (OPEN)
    bool bCommand;                    // true if no closing tag     (EMPTY)
};

typedef map<CString, CElement*> CElementMap;
typedef map<CString, CElement*>::iterator ElementsIterator;


/////////////////////////////////////////////////////////////////////////////
//  CColours - these are colours that we know about

class CColours : public CObject
{
  public:
    CString strName;  // Colour name
    COLORREF iColour; // what its RGB code is
};

typedef map<CString, CColours*> CColoursMap;
typedef map<CString, CColours*>::iterator ColoursIterator;


/////////////////////////////////////////////////////////////////////////////
//  CActiveTag - these are outstanding (unclosed) tags

class CActiveTag : public CObject
{
  public:
    CString strName; // name of tag we opened
    bool bSecure;    // was it secure mode at the time?  
    bool bNoReset;   // protected from reset?

    CActiveTag () 
      : bSecure(false), bNoReset(false)
    {}
};

typedef vector<CActiveTag*> CActiveTagList;
typedef vector<CActiveTag*>::iterator ActiveTagsIterator;


/////////////////////////////////////////////////////////////////////////////
//  CPlugin - these are world plugins

class CScriptEngine;

class CPlugin : public CObject
{
  public:
    // methods
    CPlugin (CMUSHclientDoc * pDoc);
    ~CPlugin ();

    bool SaveState ();
    DISPID GetPluginDispid (const char * sName);
    void ExecutePluginScript (const char * sName, 
                              DISPID & iRoutine);   // no arguments
    bool ExecutePluginScript (const char * sName, 
                              DISPID & iRoutine, 
                              const char * sText);  // 1 argument
    bool ExecutePluginScript (const char * sName, 
                              DISPID & iRoutine, 
                              const long arg1,      // 2 arguments
                              const string sText);
    bool ExecutePluginScript (const char * sName, 
                              DISPID & iRoutine, 
                              const long arg1,      // 3 arguments
                              const long arg2,
                              const string sText);
    bool ExecutePluginScript (const char * sName, 
                              DISPID & iRoutine, 
                              const long arg1,      // 1 number, 3 strings
                              const char * arg2,
                              const char * arg3,
                              const char * arg4);
    void ExecutePluginScript (const char * sName,
                              CString & strResult,  // taking and returning a string
                              DISPID & iRoutine);

    // data
    CString m_strName;          // name of plugin
    CString m_strAuthor;        // who wrote it
    CString m_strPurpose;       // what it does (short description)
    CString m_strDescription;   // what it does (long description)
    CString m_strScript;        // script source  (ie. from <script> tags)
    CString m_strLanguage;      // script language (eg. vbscript)
    CString m_strSource;        // include file that contains this plugin
    CString m_strDirectory;     // directory source is in (m_strSource minus the actual filename)
    CString m_strID;            // unique ID
    CTime   m_tDateWritten;     // date written
    CTime   m_tDateModified;    // date last modified
    double  m_dVersion;         // plugin version
    double  m_dRequiredVersion; // minimum MUSHclient version required
    CTime   m_tDateInstalled;   // date installed

    CScriptEngine * m_ScriptEngine; // script engine for script, if any

    CAliasMap     m_AliasMap;     // aliases     
    CAliasArray   m_AliasArray;   // array of aliases for sequencing
    CTriggerMap   m_TriggerMap;   // triggers    
    CTriggerArray m_TriggerArray; // array of triggers for sequencing
    CTimerMap     m_TimerMap;     // timers      
    CVariableMap  m_VariableMap;  // variables   
    tStringMapOfMaps m_Arrays;    // map of arrays (for scripting)

    bool m_bEnabled;              // true if active (enabled)
    CMUSHclientDoc    * m_pDoc;   // related MUSHclient document
    bool m_bSaveState;            // true to save plugin state
    bool m_bSendToScriptUsed;     // plugin sends to script
    bool m_bGlobal;               // true if plugin was loaded from global prefs
    long m_iLoadOrder;            // sequence in which plugins are processed

    // Lua note - for Lua the DISPID is a flag indicating whether or not
    // the routine exists. It is set to DISPID_UNKNOWN if the last call caused an error
    // It will be 1 if the routine exists, and DISPID_UNKNOWN if it doesn't.

    // WARNING! PHP currently uses a DISPID of zero, so that can't be used as a "not found" flag.

    DISPID m_dispid_plugin_install;         // "OnPluginInstall"    
    DISPID m_dispid_plugin_connect;         // "OnPluginConnect"    
    DISPID m_dispid_plugin_disconnect;      // "OnPluginDisconnect" 
    DISPID m_dispid_plugin_close;           // "OnPluginClose"      
    DISPID m_dispid_plugin_save_state;      // "OnPluginSaveState"  
    DISPID m_dispid_plugin_enable;          // "OnPluginEnable"     
    DISPID m_dispid_plugin_disable;         // "OnPluginDisable"    
    DISPID m_dispid_plugin_command;         // "OnPluginCommand" 
    DISPID m_dispid_plugin_command_entered; // "OnPluginCommandEntered" 
    DISPID m_dispid_plugin_get_focus;       // "OnPluginGetFocus"    
    DISPID m_dispid_plugin_lose_focus;      // "OnPluginLoseFocus" 
    DISPID m_dispid_plugin_trace;           // "OnPluginTrace" 
    DISPID m_dispid_plugin_broadcast;       // "OnPluginBroadcast" 
    DISPID m_dispid_plugin_screendraw;      // "OnPluginScreendraw" 
    DISPID m_dispid_plugin_playsound;       // "OnPluginPlaySound" 
    DISPID m_dispid_plugin_tabcomplete;     // "OnPluginTabComplete" 
    //DISPID m_dispid_plugin_tooltip;         // "OnPluginToolTip" 
    DISPID m_dispid_plugin_list_changed;    // "OnPluginListChanged"    
    DISPID m_dispid_plugin_tick;            // "OnPluginTick" 
    DISPID m_dispid_plugin_mouse_moved;     // "OnPluginMouseMoved" 

    DISPID m_dispid_plugin_send;                    // "OnPluginSend"     
    DISPID m_dispid_plugin_sent;                    // "OnPluginSent"     
    DISPID m_dispid_plugin_line_received;           // "OnPluginLineReceived"    
    DISPID m_dispid_plugin_packet_received;         // "OnPluginPacketReceived" 
    DISPID m_dispid_plugin_partial_line;            // "OnPluginPartialLine" 
    DISPID m_dispid_plugin_telnet_option;           // "OnPluginTelnetOption" 
    DISPID m_dispid_plugin_telnet_request;          // "OnPluginTelnetRequest" 
    DISPID m_dispid_plugin_telnet_subnegotiation;   // "OnPluginTelnetSubnegotiation" 
    DISPID m_dispid_plugin_IAC_GA;                  // "OnPlugin_IAC_GA" 
    DISPID m_dispid_plugin_on_world_output_resized; // "OnPluginWorldOutputResized"
    DISPID m_dispid_plugin_on_command_changed;      // "OnPluginCommandChanged"

    // MXP callbacks
    DISPID m_dispid_plugin_OnMXP_Start;       // "OnPluginMXPstart"
    DISPID m_dispid_plugin_OnMXP_Stop;        // "OnPluginMXPstop"
    DISPID m_dispid_plugin_OnMXP_OpenTag;     // "OnPluginMXPopenTag"        
    DISPID m_dispid_plugin_OnMXP_CloseTag;    // "OnPluginMXPcloseTag"
    DISPID m_dispid_plugin_OnMXP_SetVariable; // "OnPluginMXPsetVariable"
    DISPID m_dispid_plugin_OnMXP_SetEntity;   // "OnPluginMXPsetEntity"
    DISPID m_dispid_plugin_OnMXP_Error;       // "OnPluginMXPerror"

    // Chat system callbacks
    DISPID m_dispid_plugin_On_Chat_Accept;         // "OnPluginChatAccept"
    DISPID m_dispid_plugin_On_Chat_Message;        // "OnPluginChatMessage"
    DISPID m_dispid_plugin_On_Chat_MessageOut;     // "OnPluginChatMessageOut"
    DISPID m_dispid_plugin_On_Chat_Display;        // "OnPluginChatDisplay"
    DISPID m_dispid_plugin_On_Chat_NewUser;        // "OnPluginChatNewUser"
    DISPID m_dispid_plugin_On_Chat_UserDisconnect; // "OnPluginChatUserDisconnect"
};

typedef vector<CPlugin*> CPluginList;
typedef vector<CPlugin*>::iterator PluginsIterator;

// for storing map directions, and inverses of them
class CMapDirection
{
  public:
    CMapDirection (const string sDirectionToLog,
                   const string sDirectionToSend,
                   const string sReverseDirection)
      : m_sDirectionToLog  (sDirectionToLog), 
        m_sDirectionToSend (sDirectionToSend),
        m_sReverseDirection(sReverseDirection)
    {}

    // default constructor
    CMapDirection ()
    {}

    // copy constructor
    CMapDirection (const CMapDirection & m)
      : m_sDirectionToLog  (m.m_sDirectionToLog), 
        m_sDirectionToSend (m.m_sDirectionToSend),
        m_sReverseDirection(m.m_sReverseDirection)
    {}

    // operator =
    const CMapDirection & operator= (const CMapDirection & rhs)
    {
      m_sDirectionToLog   = rhs.m_sDirectionToLog;
      m_sDirectionToSend  = rhs.m_sDirectionToSend;
      m_sReverseDirection = rhs.m_sReverseDirection;
      return *this;
    }

    string m_sDirectionToLog;   // eg. "up" becomes "u"
    string m_sDirectionToSend;  // eg. "u" becomes "up"
    string m_sReverseDirection; // eg. "e" becomes "w"
};  // end of class CMapDirection



/*
// for panes

class CPaneView;

class CPane
  {
  public:

  CFrameWnd * m_pFrame;   // frame holding view
  CPaneView * m_pView;    // the view belonging to the document

  string  m_sTitle;
  long    m_iLeft;  
  long    m_iTop;  
  long    m_iWidth;
  long    m_iHeight;
  long    m_iFlags;
  
  };

  */

//typedef map<string, CPane *>::iterator PaneMapIterator;

// ----------- here used for Lua in choosing from combo-box

class CKeyValuePair
{
  public:
    CKeyValuePair (const string sValue)
      : bNumber_(false), iKey_(0.0), sValue_(sValue)
    {}

    bool   bNumber_; // true if key a number, false if a string

    string sKey_;    // key if string
    double iKey_;    // key if number?

    string sValue_;  // value 
};   // end of class  CStringValuePair
