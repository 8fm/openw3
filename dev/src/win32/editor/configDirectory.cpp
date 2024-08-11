#include "build.h"
#include "configDirectory.h"

CConfigDirectory::CConfigDirectory( const String &name, CConfigDirectory *parent )
: m_name( name )
, m_parent( parent )
{
}

CConfigDirectory::~CConfigDirectory()
{
	m_directories.ClearPtr();
}

CConfigDirectory* CConfigDirectory::GetChild( const String &name )
{
	THashMap< String, CConfigDirectory * > :: iterator result = m_directories.Find( name );
	if ( result == m_directories.End() )
	{
		return NULL;
	}
	return result->m_second;
}

void CConfigDirectory::AddChild( const String &name )
{
	if ( m_directories.Find( name ) == m_directories.End() )
	{
		CConfigDirectory *directory = new CConfigDirectory( name, this );
		m_directories.Set( name, directory );
	}
}

void CConfigDirectory::SetValue( const String &name, const String &value )
{
	m_settings.Set( name, value );
}

Bool CConfigDirectory::GetValue( const String &name, String &result )
{
	// Search in map
	THashMap< String, String >::iterator setting = m_settings.Find( name );
	if ( setting != m_settings.End() )
	{
		result = setting->m_second;
		return true;
	}

	// Not found
	return false;}

Bool CConfigDirectory::DeleteValue( const String &name, const Bool withDirectory)
{
	// Erase
	if ( m_settings.Erase( name ) )
	{
		if ( ( m_settings.Size() == 0 ) && withDirectory && m_parent )
		{
			m_parent->DeleteChild( m_name );
		}
		return true;
	}

	// Not erased
	return false;
}

Bool CConfigDirectory::DeleteChild( const String &name )
{
	return m_directories.Erase( name );
}

String CConfigDirectory::FullPath()
{
	if( m_parent )
	{
		return m_parent->FullPath() + TXT( "/" ) + m_name;
	}
	else
	{
		return m_name;
	}
}

void CConfigDirectory::Save( String &result, const String &fullPath, CConfigDirectory *skipDirectory )
{
	// This directory is skipped
	if( this == skipDirectory )
	{
		return;
	}

	// Build header
	THashMap< String, String > :: const_iterator setting;
	if ( m_parent )
	{
		result += TXT("[") + fullPath + TXT("]");
		result += String::Chr( (Char) 13 ) + String::Chr( (Char) 10 );
	}

	// Add values
	for ( setting = m_settings.Begin(); setting != m_settings.End(); ++setting )
	{
		result += setting->m_first + TXT("=") + setting->m_second;
		result += String::Chr( (Char) 13 ) + String::Chr( (Char) 10 );
	}

	// Save sub dirs
	THashMap< String, CConfigDirectory *> :: const_iterator directory;
	if ( m_parent )
	{
		for ( directory = m_directories.Begin(); directory != m_directories.End(); ++directory )
		{
			directory->m_second->Save( result, fullPath + TXT("/") + directory->m_first, skipDirectory );
		}
	}
	else
	{
		for ( directory = m_directories.Begin(); directory != m_directories.End(); ++directory )
		{
			directory->m_second->Save( result, directory->m_first, skipDirectory );
		}
	}

	// Remove unused memory
	m_directories.Shrink();
	m_settings.Shrink();
}

void CConfigDirectory::Shrink()
{
	// Remove unused memory
	m_directories.Shrink();
	m_settings.Shrink();

	// Recurse
	THashMap< String, CConfigDirectory *> :: const_iterator directory;
	for ( directory = m_directories.Begin(); directory != m_directories.End(); ++directory )
	{
		directory->m_second->Shrink();
	}
}
