/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "configFileManager.h"
#include "filePath.h"
#include "filesys.h"

CConfigManager* GConfig = NULL;

CConfigSection::CConfigSection( CConfigFile* file )
	: m_file( file )
{
}

CConfigSection::~CConfigSection()
{
}

void CConfigSection::Clear()
{
	// Remove all data
	m_items.Clear();

	// Mark file as modified
	m_file->MarkModified();
}

Bool CConfigSection::ReadValue( const String& key, String& value ) const
{
	// Find value
	const TDynArray< String >* foundValues = m_items.FindPtr( key );
	if ( foundValues && foundValues->Size() )
	{
		value = (*foundValues)[0];
		return true;
	}

	// Not found
	return false;
}

Bool CConfigSection::ReadValues( const String& key, TDynArray< String > & values ) const
{
	// Find value
	const TDynArray< String >* foundValues = m_items.FindPtr( key );
	if ( foundValues && foundValues->Size() )
	{
		values = *foundValues;
		return true;
	}

	// Not found
	return false;
}

Bool CConfigSection::WriteValue( const String& key, const String& value, Bool append )
{
	// Empty key
	if ( key.Empty() )
	{
		return false;
	}

	// Do not append empty values
	if ( append && value.Empty() )
	{
		return false;
	}

	// Get the values
	TDynArray< String >* values = m_items.FindPtr( key );
	if ( values )
	{
		// Append only non empty values
		if ( append )
		{
			// Append new value
			values->PushBack( value );
		}
		else
		{
			// Emit value
			values->Resize( 1 );
			(*values)[0] = value;
		}
	}
	else
	{
		// Add new value in section
		TDynArray< String > values( 1 );
		values[0] = value;
		m_items.Insert( key, values );
	}

	// Mark file as modified
	m_file->MarkModified();
	return true;
}

Bool CConfigSection::RemoveValues( const String& key )
{
	// Remove whole key from config
	if ( !m_items.Erase( key ) )
	{
		return false;
	}

	// Mark file as modified
	m_file->MarkModified();
	return true;
}

Bool CConfigSection::RemoveValue( const String& key, String& value )
{
	// Find key array
	TDynArray< String >* values = m_items.FindPtr( key );
	if ( !values )
	{
		return false;
	}

	// Remove value
	if ( !values->Remove( value ) )
	{
		return false;
	}

	// Last value remove, remove the whole definition
	if ( !values->Size() )
	{
		m_items.Erase( key );
	}

	// Mark file as modified
	m_file->MarkModified();
	return true;
}

CConfigFile::CConfigFile( const String& shortName, const String& filePath )
	: m_filePath( filePath )
	, m_shortName( shortName )
	, m_isModified( false )
{
	// Load the file
	Read();
}

CConfigFile::~CConfigFile()
{
	m_sections.ClearPtr();
}

void CConfigFile::MarkModified()
{
	m_isModified = true;
}

Bool CConfigFile::Read()
{
	// Flush before reloading
	if ( m_isModified )
	{
		Write();
	}

	// Load the base file first
	CFilePath baseFilePath( m_filePath );
	baseFilePath.PushDirectory( TXT("base") );
	Read( baseFilePath.ToString(), true );

	// Load the local file. Overwrite the values.
	return Read( m_filePath, false );
}

Bool CConfigFile::Parse( const String &filePath, Bool addValues ) 
{
	// Load to string
	String configString;

	if ( !GFileManager->LoadFileToString( filePath, configString, true ) )
	{
		return false;
	}

	// Empty config file
	if ( configString.Empty() )
	{
		return false;
	}

	// Parse lines
	CConfigSection* section = NULL;
	const Char* cur = configString.AsChar();
	while ( *cur )
	{
		Char ch = *cur;

		// Parse white space && non-ansi chars
		if ( ch <= 32 || ch > 127 )
		{
			cur++;
			continue;
		}

		// Section name
		if ( ch == '[' )
		{
			cur++;

			String sectionName;
			while ( *cur )
			{
				if ( *cur <= 32 )
				{
					WARN_CORE( TXT("Error parsing config file '%ls'"), filePath.AsChar() );
					return false;
				}

				if ( *cur == ']' )
				{
					cur++;
					break;
				}

				// Config name
				sectionName += String::Chr( *cur++ );
			}

			// Create section
			section = GetSection( sectionName, true );
			if ( !section )
			{
				WARN_CORE( TXT("Error parsing config file '%ls': no section '%ls'"), filePath.AsChar(), sectionName.AsChar() );
				return false;
			}

			continue;
		}

		// Key=Value pair, parse till the end of line
		Bool isKey = true;
		String key, value;
		while ( *cur )
		{
			if ( *cur < 32 )
			{
				break;
			}

			if ( *cur == '=' && isKey )
			{
				isKey = false;
				cur++;
				continue;
			}

			// Parse key or value name
			if ( isKey )
			{
				key += String::Chr( *cur );
			}
			else
			{
				value += String::Chr( *cur );
			}

			cur++;
		}

		// Empty key
		if ( key.Empty() )
		{
			WARN_CORE( TXT("Error parsing config file '%ls': empty key"), filePath.AsChar() );
			return false;
		}

		// Add to section
		if ( !section )
		{
			WARN_CORE( TXT("Error parsing config file '%ls': key '%ls' without section"), filePath.AsChar(), key.AsChar() );
			return false;
		}

		if ( addValues )
		{
			// Add to values
			section->WriteValue( key, value, true );
		}
		else
		{
			// Clear repeated key
			section->RemoveValues( key );
		}
	}

	// Parsed
	m_isModified = false;

	return true;
}

Bool CConfigFile::Read( const String& filePath, Bool base/*=false*/ )
{
	if ( base )
	{
		// Clear current config
		m_sections.ClearPtr();
		m_isModified = false;
	}
	else
	{
		// Clear only repeated keys
		Parse( filePath, false );
	}

	// Add new values from file
	return Parse( filePath, true );
}

Bool CConfigFile::Write()
{
	// Format output string
	String str;
	for ( auto i=m_sections.Begin(); i!=m_sections.End(); ++i )
	{
		// Section name
		str += String::Printf( TXT("[%s]\r\n"), i->m_first.AsChar() );

		// Section values
		const THashMap< String, TDynArray< String > >& values = i->m_second->GetItems();
		for ( THashMap< String, TDynArray< String > >::const_iterator j=values.Begin(); j!=values.End(); ++j )
		{
			// Values
			for ( Uint32 k=0; k<j->m_second.Size(); k++ )
			{
				str += String::Printf( TXT("%s=%s\r\n"), j->m_first.AsChar(), j->m_second[k].AsChar() );
			}

		}

		// Separation
		str += TXT("\r\n");
	}

	// Save string to file
	GFileManager->SaveStringToFile( m_filePath, str );

	// Not modified any more
	m_isModified = false;
	return true;
}

CConfigSection* CConfigFile::GetSection( const String& name, Bool createIfNotFound/*=false*/ )
{
	// Empty section names are not supported
	if ( name.Empty() )
	{
		return NULL;
	}

	// Find existing section
	CConfigSection* section = NULL;
	if ( !m_sections.Find( name, section ) )
	{
		// Create new section
		if ( createIfNotFound )
		{
			section = new CConfigSection( this );
			m_sections.Insert( name, section );
			m_isModified = true;
		}
	}

	// Return created or found section
	return section;
}

CConfigManager::CConfigManager( const String& configDir )
	: m_configDir( configDir )
{
}

CConfigManager::~CConfigManager()
{
	// Flush modified only
	Save( false );

	// Remove
	m_files.ClearPtr();
}

void CConfigManager::SetAlias( const String& alias, const String& file )
{
	// Flush files
	Save( false );

	// Cleanup
	m_files.ClearPtr();

	// Set alias
	m_aliases.Set( alias, file );
}

void CConfigManager::Save( Bool flushAll /*=false*/ )
{
	RED_UNUSED( flushAll );
#if defined( RED_PLATFORM_WINPC )
	LOG_CORE( TXT("Saving configuration") );

	// Save all configuration files
	for ( TDynArray< CConfigFile* >::iterator i=m_files.Begin(); i!=m_files.End(); ++i )
	{
		CConfigFile* file = *i;
		if ( file->IsModified() || flushAll )
		{
			file->Write();
		}
	}
#endif
}

CConfigFile* CConfigManager::GetFile( const Char* category )
{
	// No category given
	if ( !category || !category[0] )
	{
		return NULL;
	}

	// Search for existing file
	for ( Uint32 i=0; i<m_files.Size(); i++ )
	{
		CConfigFile* file = m_files[i];
		if ( file->GetShortName().EqualsNC( category ) )
		{
			return file;
		}
	}

	// Translate file name using alias table
	String fileName = category;
	m_aliases.Find( category, fileName );

	// Invalid category name :)
	Char fullConfigFilePath[ 256 ];
	Red::System::SNPrintF( fullConfigFilePath, 256, TXT("%ls%ls.ini"), m_configDir.AsChar(), fileName.AsChar() );
//	wsprintf( fullConfigFilePath, TXT("%s%s.ini"), m_configDir.AsChar(), fileName.AsChar() );

	// Add to list
	CConfigFile* file = new CConfigFile( category, fullConfigFilePath );
	m_files.PushBack( file );
	return file;
}

Bool CConfigManager::EnsureConfigFilesExist( const String& templatePath )
{
	Bool result = true;

	TDynArray< String > userConfigFiles;
	GFileManager->FindFiles( m_configDir, TXT( "*.ini" ), userConfigFiles, true );

	TDynArray< String > templateConfigFiles;
	GFileManager->FindFiles( templatePath, TXT( "*.ini" ), templateConfigFiles, true );

	for ( TDynArray< String >::iterator templateConfigIter = templateConfigFiles.Begin();
		templateConfigIter != templateConfigFiles.End(); ++templateConfigIter )
	{
		String& templateConfigFile = *templateConfigIter;
		String expectedUserConfigFile = m_configDir + templateConfigFile.StringAfter( templatePath );
		if ( userConfigFiles.Exist( expectedUserConfigFile ) == false )
		{
			GFileManager->CreatePath( expectedUserConfigFile );
			result &= GFileManager->CopyFile( templateConfigFile, expectedUserConfigFile, false );
		}
	}

	return result;
}
