#pragma once

class CConfigDirectory
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	String									m_name;
	CConfigDirectory*						m_parent;
	THashMap< String, CConfigDirectory * >		m_directories;
	THashMap< String, String >					m_settings;

public:
	//! Get setting values
	RED_INLINE THashMap< String, String >& GetSettings(){ return m_settings; }

	//! Get subdirectories
	RED_INLINE THashMap< String, CConfigDirectory* >& GetDirectories() { return m_directories; }

public:
	CConfigDirectory( const String &name, CConfigDirectory *parent = NULL );
	~CConfigDirectory();

	// Gets child directory with the given name or NULL if there is no such
	CConfigDirectory* GetChild( const String &name );

	// Sets value of setting with given name
	void SetValue( const String &name, const String &value );

	// Gets value of a setting with the given name; returns true if successfully
	// retrieved value
	Bool GetValue( const String &name, String &result );

	// Deletes value of a setting with the given name; returns true if successfully
	// deleted value
	Bool DeleteValue( const String &name, const Bool withDirectory );

	// Adds a child directory with a given name or does nothing, if it already exists
	void AddChild( const String &name );

	// Deletes child directory with the given name
	Bool DeleteChild( const String &name );

	// Saves contents of this directory and all subdirectories to a given string
	void Save( String &result, const String &fullPath, CConfigDirectory *skipDirectory = NULL );

	// Get full path
	String FullPath();

	// Reuse wasted memory
	void Shrink();
};