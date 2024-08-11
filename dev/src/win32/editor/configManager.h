#pragma once

class CConfigDirectory;

#define CONFIG_FILE_MAIN				"LavaEditor2.ini"
#define CONFIG_FILE_SESSIONS			"LavaEditor2.sessions.ini"
#define CONFIG_FILE_COMMON				"LavaEditor2.common.ini"

class ISavableToConfig;


class CConfigurationScopedPathSetter
{
public:
	RED_FORCE_INLINE CConfigurationScopedPathSetter( class CConfigurationManager& manager, const String& newPath );
	RED_FORCE_INLINE ~CConfigurationScopedPathSetter();

private:
	CConfigurationManager& m_manager;
};


class CConfigurationManager abstract
{
	friend class ISavableToConfig;
protected:
	String								m_filePathMain;
	CConfigDirectory*					m_root;
	CConfigDirectory*					m_current;
	TDynArray< CConfigDirectory* >		m_currentToRestore;
	TDynArray< ISavableToConfig* >		m_savables;
	Red::Threads::CMutex				m_mutex;
	Red::Threads::CMutex				m_saveAllMutex;

	void AddSavable( ISavableToConfig* );
	void RemoveSavable( ISavableToConfig* );

public:
	// Sets value of the setting on the given path
	void Write( const String &path, const String &value );
	void Write( const String &path, const Int32 value );
	void Write( const String &path, const Float value );

	// Reads value of the setting on the given path; returns true if successful 
	Bool Read( const String &path, String *value );
	Int32 Read( const String &path, Int32 default_value );
	Float Read( const String &path, Float default_value );
	String Read( const String &path, const String &default_value );

	// Deletes setting with the given name (not path!) in the current directory; if
	// deleteGroupIfEmpty is true, directory is deleted if there are no more settings
	Bool DeleteEntry( const String &name, const Bool withDirectory = true );

	void DeleteAllEntriesFromCurrentDir();

	Bool DeleteDirectory( const String &path );

	// Retrieves all direct settings inside the current directory
	Bool AllSettings( TDynArray< TPair< String, String > > &settings );

	// Retrieves all direct directories inside the current directory
	Bool GetDirectories( TDynArray< String > &directories ) const;

	// Take all options and save
	virtual void SaveAll( bool withEvent = true );

protected:
	friend class CConfigurationScopedPathSetter;

	// adds a setting stored in a string in a form "name=value" to the current directory
	void AddSetting( const String &setting );

	CConfigDirectory* AddDirectory( const String &path );
	CConfigDirectory* AddDirectory( const TDynArray< String > &path );

	CConfigDirectory *GetDirectory( const TDynArray< String > &path );
	void ProcessLine( const String &line );
	virtual void Load();
	void Load( const String &fileName );
	virtual void Save();

private:
	// use CConfigurationScopedPathSetter to locally alter the path
	void SetPath( const String &path );
	void RemeberCurrentPath();
	void RestoreCurrentPath();

public:
	static const Uint32 MAX_CONFIG_PATH = 512;

	//! GConfig like interface - read element
	template< class T >
	Bool ReadParam( const Char* category, const Char* section, const Char* key, T& param )
	{
		CConfigurationScopedPathSetter pathSetter( *this, TXT("/EditorConfig") );

		Char fullPath[ MAX_CONFIG_PATH ];
		Red::System::SNPrintF( fullPath, MAX_CONFIG_PATH, TXT("%ls/%ls/%ls"), category, section, key );

		String ret;
		if ( Read( fullPath, &ret ) )
		{
			return FromString<T>( ret, param );
		}

		return false;
	}

	//! GConfig like interface - read array of elements
	template< class T >
	Bool ReadParams( const Char* category, const Char* section, const Char* key, TDynArray<T>& param )
	{
		CConfigurationScopedPathSetter pathSetter( *this, TXT("/EditorConfig") );

		Char fullPath[ MAX_CONFIG_PATH ];
		Red::System::SNPrintF( fullPath, MAX_CONFIG_PATH, TXT("%ls/%ls/%ls/Count"), category, section, key );

		String ret;
		if ( !Read( fullPath, &ret ) )
			return false;

		Int32 count = 0;
		 if ( !FromString<Int32>( ret, count ) )
			 return false;

		for ( Int32 i=0; i<count; ++i )
		{
			Red::System::SNPrintF( fullPath, MAX_CONFIG_PATH, TXT("%ls/%ls/%ls/Elem%d"), category, section, key, i );

			String ret;
			if ( Read( fullPath, &ret ) )
			{
				T elem;
				if ( !FromString<T>( ret, elem ) )
				{
					param.PushBack( elem );
				}
			}
		}

		return true;
	}

	//! Save parameter
	template< class T >
	Bool WriteParam( const Char* category, const Char*section, const Char* key, const T& param )
	{
		CConfigurationScopedPathSetter pathSetter( *this, TXT("/EditorConfig") );

		Char fullPath[ MAX_CONFIG_PATH ];
		Red::System::SNPrintF( fullPath, MAX_CONFIG_PATH, TXT("%ls/%ls/%ls"), category, section, key );

		String value = ToString<T>( param );
		Write( fullPath,	value );
		return true;
	}

	//! Save parameter array
	template< class T >
	Bool WriteParamArray( const Char* category, const Char* section, const Char* key, const TDynArray<T>& param )
	{
		CConfigurationScopedPathSetter pathSetter( *this, TXT("/EditorConfig") );

		Char fullPath[ MAX_CONFIG_PATH ];

		// save count
		Red::System::SNPrintF( fullPath, MAX_CONFIG_PATH, TXT("%ls/%ls/%ls/Count"), category, section, key );
		Write( fullPath, (Int32)param.Size() );

		// save elements
		for ( Int32 i=0; i<(Int32)param.Size(); ++i )
		{
			Red::System::SNPrintF( fullPath, MAX_CONFIG_PATH, TXT("%ls/%ls/%ls/Elem%d"), category, section, key, i );

			const String value = ToString<T>( param[i] );
			Write( fullPath, value );
		}

		return true;
	}
};

class CUserConfigurationManager : public CConfigurationManager
{
	friend class ISavableToConfig;
private:
	String								m_filePathSessions;
	Red::Threads::CMutex				m_saveSessionMutex;
	String								m_sessionPath;

public:
	CUserConfigurationManager();
	~CUserConfigurationManager();

	virtual void SaveAll( bool withEvent = true );
	void SaveSession();

	void SetSessionPath( String path ) { m_sessionPath = path; }
	String GetSessionPath() const;

protected:
	virtual void Load();
	virtual void Save();
};

class CCommonConfigurationManager : public CConfigurationManager
{
	friend class ISavableToCommonConfig;

public:
	CCommonConfigurationManager();
	~CCommonConfigurationManager();
};

typedef TSingleton<CUserConfigurationManager> SUserConfigurationManager;
typedef TSingleton<CCommonConfigurationManager> SCommonConfigurationManager;

class ISavableToConfig
{
	Red::Threads::CMutex m_sessionMutex;
public:
	ISavableToConfig();
	virtual ~ISavableToConfig();
	virtual void SaveOptionsToConfig(){}
	virtual void LoadOptionsFromConfig(){}

	virtual void SaveSession( CConfigurationManager &config ){}
	virtual void RestoreSession( CConfigurationManager &config ){}

	void RestoreSession();
	void SaveSession();
};

class ISavableToCommonConfig : public ISavableToConfig
{
public:
	ISavableToCommonConfig();
	virtual ~ISavableToCommonConfig();
};

RED_FORCE_INLINE CConfigurationScopedPathSetter::CConfigurationScopedPathSetter( CConfigurationManager& manager, const String& newPath )
	: m_manager( manager )
{
	m_manager.RemeberCurrentPath();
	m_manager.SetPath( newPath );
}

RED_FORCE_INLINE CConfigurationScopedPathSetter::~CConfigurationScopedPathSetter()
{
	m_manager.RestoreCurrentPath();
}

