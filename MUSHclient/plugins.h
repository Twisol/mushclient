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
    CString m_strCallingPluginID; // during a CallPlugin - the ID of the calling plugin

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
typedef CPluginList::iterator PluginsIterator;