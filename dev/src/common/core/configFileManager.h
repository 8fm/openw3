/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "sortedmap.h"
#include "string.h"
#include "math.h"
#include "stringConversion.h"

class CConfigFile;

typedef THashMap< String, TDynArray< String > >	TConfigSectionKeys;

/// Section of configuration file
class CConfigSection
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	TConfigSectionKeys		m_items;		//!< Items in this section
	CConfigFile*			m_file;			//!< File this section is in

public:
	//! Get the configuration items
	RED_INLINE const TConfigSectionKeys& GetItems() const { return m_items; }

public:
	CConfigSection( CConfigFile* file );
	~CConfigSection();

	//! Clear section ( remove all key-value paris )
	void Clear();

	//! Read value ( the first one if multiple are defined )
	Bool ReadValue( const String& key, String& value ) const;

	//! Read all values for given key
	Bool ReadValues( const String& key, TDynArray< String > & values ) const;

	//! Write value
	Bool WriteValue( const String& key, const String& value, Bool append );

	//! Remove all values for entry
	Bool RemoveValues( const String& key );

	//! Remove entry form config
	Bool RemoveValue( const String& key, String& value );
};

/// Configuration file
class CConfigFile
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	String								m_shortName;		//!< Short config file name
	String								m_filePath;			//!< Path to configuration file
	TSortedMap< String, CConfigSection* >		m_sections;			//!< Sections in the config file
	Bool								m_isModified;		//!< File is modified ( needs to be saved )

public:
	//! Get config file path to the ini file
	RED_INLINE const String& GetFilePath() const { return m_filePath; }

	//! Get short name
	RED_INLINE const String& GetShortName() const { return m_shortName; }

	//! Is this file modified
	RED_INLINE Bool IsModified() const { return m_isModified; }

public:
	CConfigFile( const String& shortName, const String& filePath );
	~CConfigFile();

	//! Mark configuration file as modified
	void MarkModified();

	//! Load configuration
	Bool Read();

	//! Load configuration
	Bool Read( const String& filePath, Bool base = false );

	//! Write configuration file
	Bool Write();

public:
	//! Get section
	CConfigSection* GetSection( const String& name, Bool createIfNotFound=false );

	//! Get sections
	RED_INLINE const TSortedMap< String, CConfigSection* >& GetSections() const { return m_sections; }

private:
	//! Load configuration
	Bool Parse( const String& filePath, Bool addValues );
};

/// Configuration files
class CConfigManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	String						m_configDir;		//!< Directory with main configuration files
	TDynArray< CConfigFile* >	m_files;			//!< Configuration files
	THashMap< String, String >	m_aliases;			//!< Config aliases

private:
	RED_INLINE Bool ParseValue( const String& in, String& out ) const
	{
		const Char* stream = in.AsChar();
		return GParseString( stream, out );
	}

	template< typename T >
	RED_INLINE Bool ParseValue( const String& in, T& out ) const
	{
		return FromString< T >( in, out );
	}

public:
	CConfigManager( const String& configDir );
	~CConfigManager();

	//! Set config alias
	void SetAlias( const String& alias, const String& file );

	//! Save configuration files
	void Save( Bool flushAll = false );

	//! Get configuration file for given category
	CConfigFile* GetFile( const Char* category );

	//! Checks if all needed config files are in config dir and copies them from config templates if needed
	Bool EnsureConfigFilesExist( const String& templatePath );

public:
	//! Load parameter
	template< class T >
	Bool ReadParam( const Char* category, const String& section, const String& key, T& param )
	{
		CConfigFile* configFile = GetFile( category );
		if ( configFile )
		{
			CConfigSection* configSection = configFile->GetSection( section );
			if ( configSection )
			{
				String value;
				if ( configSection->ReadValue( key, value ) )
				{
					return ParseValue( value, param );
				}
			}
		}

		// Not found
		return false;
	}

	//! Save parameter
	template< class T >
	Bool WriteParam( const Char* category, const Char*section, const Char* key, const T& param )
	{
		CConfigFile* configFile = GetFile( category );
		if ( configFile )
		{
			CConfigSection* configSection = configFile->GetSection( section, true );
			if ( configSection )
			{
				String value = ToString<T>( param );
				return configSection->WriteValue( key, value, false );
			}
		}

		// Not found
		return false;
	}
};

//! Load bool parameter
template<>
RED_INLINE Bool CConfigManager::ReadParam( const Char* category, const String& section, const String& key, Bool& param )
{
	CConfigFile* configFile = GetFile( category );
	if ( configFile )
	{
		CConfigSection* configSection = configFile->GetSection( section );
		if ( configSection )
		{
			String value;
			if ( configSection->ReadValue( key, value ) )
			{
				// Boolean true
				if ( value.EqualsNC( TXT("true") ) )
				{
					param = true;
					return true;
				}

				// Boolean false
				if ( value.EqualsNC( TXT("false") ) )
				{
					param = false;
					return true;
				}

				// We also accept the 0/1 values here
				Int32 numValue = 0;
				if ( FromString< Int32 >( value, numValue ) )
				{
					param = numValue > 0;
					return true;
				}

				// Not parsed
				return false;
			}
		}
	}

	// Not found
	return false;
}

/// Low-level configuration manager
extern CConfigManager* GConfig;
